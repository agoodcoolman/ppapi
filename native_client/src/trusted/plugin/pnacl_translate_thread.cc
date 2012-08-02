// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "native_client/src/trusted/plugin/pnacl_translate_thread.h"

#include "native_client/src/trusted/desc/nacl_desc_wrapper.h"
#include "native_client/src/trusted/plugin/plugin.h"
#include "native_client/src/trusted/plugin/pnacl_resources.h"
#include "native_client/src/trusted/plugin/srpc_params.h"
#include "native_client/src/trusted/plugin/temporary_file.h"
#include "native_client/src/trusted/plugin/utility.h"

namespace plugin {

PnaclTranslateThread::PnaclTranslateThread() : subprocesses_should_die_(false),
                                               current_rev_interface_(NULL),
                                               done_(false),
                                               manifest_(NULL),
                                               ld_manifest_(NULL),
                                               obj_file_(NULL),
                                               nexe_file_(NULL),
                                               coordinator_error_info_(NULL),
                                               resources_(NULL),
                                               plugin_(NULL) {
  NaClXMutexCtor(&subprocess_mu_);
  NaClXMutexCtor(&cond_mu_);
  NaClXCondVarCtor(&buffer_cond_);
}

void PnaclTranslateThread::RunTranslate(
    const pp::CompletionCallback& finish_callback,
    const Manifest* manifest,
    const Manifest* ld_manifest,
    TempFile* obj_file,
    TempFile* nexe_file,
    ErrorInfo* error_info,
    PnaclResources* resources,
    Plugin* plugin) {
  PLUGIN_PRINTF(("PnaclStreamingTranslateThread::RunTranslate)\n"));
  manifest_ = manifest;
  ld_manifest_ = ld_manifest;
  obj_file_ = obj_file;
  nexe_file_ = nexe_file;
  coordinator_error_info_ = error_info;
  resources_ = resources;
  plugin_ = plugin;

  // Invoke llc followed by ld off the main thread.  This allows use of
  // blocking RPCs that would otherwise block the JavaScript main thread.
  report_translate_finished_ = finish_callback;
  translate_thread_.reset(new NaClThread);
  if (translate_thread_ == NULL) {
    TranslateFailed("could not allocate thread struct.");
    return;
  }
  const int32_t kArbitraryStackSize = 128 * 1024;
  if (!NaClThreadCreateJoinable(translate_thread_.get(),
                                DoTranslateThread,
                                this,
                                kArbitraryStackSize)) {
    TranslateFailed("could not create thread.");
    translate_thread_.reset(NULL);
  }
}

// Called from main thread to send bytes to the translator.
void PnaclTranslateThread::PutBytes(std::vector<char>* bytes,
                                             int count) {
  PLUGIN_PRINTF(("PutBytes, this %p bytes %p, size %d, count %d\n", this, bytes,
                 bytes ? bytes->size(): 0, count));
  size_t buffer_size = 0;
  // If we are done (error or not), Signal the translation thread to stop.
  if (count <= PP_OK) {
    NaClXMutexLock(&cond_mu_);
    done_ = true;
    NaClXCondVarSignal(&buffer_cond_);
    NaClXMutexUnlock(&cond_mu_);
    return;
  }

  CHECK(bytes != NULL);
  // Ensure that the buffer we send to the translation thread is the right size
  // (count can be < the buffer size). This can be done without the lock.
  buffer_size = bytes->size();
  bytes->resize(count);

  NaClXMutexLock(&cond_mu_);

  data_buffers_.push_back(std::vector<char>());
  bytes->swap(data_buffers_.back()); // Avoid copying the buffer data.

  NaClXCondVarSignal(&buffer_cond_);
  NaClXMutexUnlock(&cond_mu_);

  // Ensure the buffer we send back to the coordinator is the expected size
  bytes->resize(buffer_size);
}

NaClSubprocess* PnaclTranslateThread::StartSubprocess(
    const nacl::string& url_for_nexe,
    const Manifest* manifest,
    ErrorInfo* error_info) {
  PLUGIN_PRINTF(("PnaclTranslateThread::StartSubprocess (url_for_nexe=%s)\n",
                 url_for_nexe.c_str()));
  nacl::DescWrapper* wrapper = resources_->WrapperForUrl(url_for_nexe);
  nacl::scoped_ptr<NaClSubprocess> subprocess(
      plugin_->LoadHelperNaClModule(wrapper, manifest, error_info));
  if (subprocess.get() == NULL) {
    PLUGIN_PRINTF((
        "PnaclTranslateThread::StartSubprocess: subprocess creation failed\n"));
    return NULL;
  }
  return subprocess.release();
}

void WINAPI PnaclTranslateThread::DoTranslateThread(void* arg) {
  PnaclTranslateThread* translator =
      reinterpret_cast<PnaclTranslateThread*>(arg);
  translator->DoTranslate();
}

void PnaclTranslateThread::DoTranslate() {
  ErrorInfo error_info;
  nacl::scoped_ptr<NaClSubprocess> llc_subprocess(
      StartSubprocess(PnaclUrls::GetLlcUrl(), manifest_, &error_info));
  if (llc_subprocess == NULL) {
    TranslateFailed("Compile process could not be created: " +
                    error_info.message());
    return;
  }
  // Run LLC.
  SrpcParams params;
  nacl::DescWrapper* llc_out_file = obj_file_->write_wrapper();
  PluginReverseInterface* llc_reverse =
      llc_subprocess->service_runtime()->rev_interface();
  llc_reverse->AddTempQuotaManagedFile(obj_file_->identifier());
  RegisterReverseInterface(llc_reverse);

  if (!llc_subprocess->InvokeSrpcMethod("StreamInit",
                                        "h",
                                        &params,
                                        llc_out_file->desc())) {
    // StreamInit returns an error message if the RPC fails.
    TranslateFailed(nacl::string("Stream init failed: ") +
                    nacl::string(params.outs()[0]->arrays.str));
    return;
  }

  PLUGIN_PRINTF(("PnaclCoordinator: StreamInit successful\n"));

  // llc process is started.
  while(!done_ || data_buffers_.size() > 0) {
    NaClXMutexLock(&cond_mu_);
    while(!done_ && data_buffers_.size() == 0) {
      NaClXCondVarWait(&buffer_cond_, &cond_mu_);
    }
    PLUGIN_PRINTF(("PnaclTranslateThread awake, done %d, size %d\n",
                   done_, data_buffers_.size()));
    if (data_buffers_.size() > 0) {
      std::vector<char> data;
      data.swap(data_buffers_.front());
      data_buffers_.pop_front();
      NaClXMutexUnlock(&cond_mu_);
      PLUGIN_PRINTF(("StreamChunk\n"));
      if (!llc_subprocess->InvokeSrpcMethod("StreamChunk",
                                            "C",
                                            &params,
                                            &data[0],
                                            data.size())) {
        TranslateFailed("Compile stream chunk failed.");
        return;
      }
      PLUGIN_PRINTF(("StreamChunk Successful\n"));
    } else {
      NaClXMutexUnlock(&cond_mu_);
    }
    if (SubprocessesShouldDie()) {
      TranslateFailed("Stopped by coordinator.");
      return;
    }
  }
  PLUGIN_PRINTF(("PnaclTranslateThread done with chunks\n"));
  // Finish llc.
  if(!llc_subprocess->InvokeSrpcMethod("StreamEnd",
                                       "",
                                       &params)) {
    PLUGIN_PRINTF(("PnaclTranslateThread StreamEnd failed\n"));
    TranslateFailed(params.outs()[3]->arrays.str);
    return;
  }
  // LLC returns values that are used to determine how linking is done.
  int is_shared_library = (params.outs()[0]->u.ival != 0);
  nacl::string soname = params.outs()[1]->arrays.str;
  nacl::string lib_dependencies = params.outs()[2]->arrays.str;
  PLUGIN_PRINTF(("PnaclCoordinator: compile (translator=%p) succeeded"
                 " is_shared_library=%d, soname='%s', lib_dependencies='%s')\n",
                 this, is_shared_library, soname.c_str(),
                 lib_dependencies.c_str()));

  // Shut down the llc subprocess.
  RegisterReverseInterface(NULL);
  llc_subprocess.reset(NULL);
  if (SubprocessesShouldDie()) {
    TranslateFailed("stopped by coordinator.");
    return;
  }

  if(!RunLdSubprocess(is_shared_library, soname, lib_dependencies)) {
    return;
  }
  pp::Core* core = pp::Module::Get()->core();
  core->CallOnMainThread(0, report_translate_finished_, PP_OK);
}

bool PnaclTranslateThread::RunLdSubprocess(int is_shared_library,
                                           const nacl::string& soname,
                                           const nacl::string& lib_dependencies
                                           ) {
  ErrorInfo error_info;
  nacl::scoped_ptr<NaClSubprocess> ld_subprocess(
      StartSubprocess(PnaclUrls::GetLdUrl(), ld_manifest_, &error_info));
  if (ld_subprocess == NULL) {
    TranslateFailed("Link process could not be created: " +
                    error_info.message());
    return false;
  }
  // Run LD.
  SrpcParams params;

  // Reset object file for reading first.
  if (!obj_file_->Reset()) {
    TranslateFailed("Link process could not reset object file");
    return false;
  }
  nacl::DescWrapper* ld_in_file = obj_file_->read_wrapper();
  nacl::DescWrapper* ld_out_file = nexe_file_->write_wrapper();
  PluginReverseInterface* ld_reverse =
      ld_subprocess->service_runtime()->rev_interface();
  ld_reverse->AddTempQuotaManagedFile(nexe_file_->identifier());
  RegisterReverseInterface(ld_reverse);
  if (!ld_subprocess->InvokeSrpcMethod("RunWithDefaultCommandLine",
                                       "hhiss",
                                       &params,
                                       ld_in_file->desc(),
                                       ld_out_file->desc(),
                                       is_shared_library,
                                       soname.c_str(),
                                       lib_dependencies.c_str())) {
    TranslateFailed("link failed.");
    return false;
  }
  PLUGIN_PRINTF(("PnaclCoordinator: link (translator=%p) succeeded\n",
                 this));
  // Shut down the ld subprocess.
  RegisterReverseInterface(NULL);
  ld_subprocess.reset(NULL);
  if (SubprocessesShouldDie()) {
    TranslateFailed("stopped by coordinator.");
    return false;
  }
  return true;
}

void PnaclTranslateThread::TranslateFailed(const nacl::string& error_string) {
  PLUGIN_PRINTF(("PnaclTranslateThread::TranslateFailed (error_string='%s')\n",
                 error_string.c_str()));
  pp::Core* core = pp::Module::Get()->core();
  if (coordinator_error_info_->message().empty()) {
    // Only use our message if one hasn't already been set by the coordinator
    // (e.g. pexe load failed).
    coordinator_error_info_->SetReport(ERROR_UNKNOWN,
                                       nacl::string("PnaclCoordinator: ") +
                                       error_string);
  }
  core->CallOnMainThread(0, report_translate_finished_, PP_ERROR_FAILED);
}

// This synchronization method (using the pointer directly in the
// translation thread, setting a copy here, and calling shutdown on the
// main thread) is safe only because only the translation thread sets
// the copy, and the shutdown method is thread-safe. This method must be
// called on the translation thread before any RPCs are called, and called
// again with NULL before the object is destroyed.
void PnaclTranslateThread::RegisterReverseInterface(
    PluginReverseInterface *iface) {
  nacl::MutexLocker ml(&subprocess_mu_);
  current_rev_interface_ = iface;
}


bool PnaclTranslateThread::SubprocessesShouldDie() {
  nacl::MutexLocker ml(&subprocess_mu_);
  return subprocesses_should_die_;
}

void PnaclTranslateThread::SetSubprocessesShouldDie() {
  PLUGIN_PRINTF(("PnaclTranslateThread::SetSubprocessesShouldDie\n"));
  NaClXMutexLock(&subprocess_mu_);
  subprocesses_should_die_ = true;
  if (current_rev_interface_) {
    current_rev_interface_->ShutDown();
    current_rev_interface_ = NULL;
  }
  NaClXMutexUnlock(&subprocess_mu_);
  nacl::MutexLocker ml(&cond_mu_);
  done_ = true;
  NaClXCondVarSignal(&buffer_cond_);
}

PnaclTranslateThread::~PnaclTranslateThread() {
  PLUGIN_PRINTF(("~PnaclTranslateThread (translate_thread=%p)\n", this));
  SetSubprocessesShouldDie();
  NaClThreadJoin(translate_thread_.get());
  PLUGIN_PRINTF(("~PnaclTranslateThread joined\n"));
  NaClMutexDtor(&subprocess_mu_);
}

} // namespace plugin
