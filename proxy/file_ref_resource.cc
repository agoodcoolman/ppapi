// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/file_ref_resource.h"

#include "ppapi/c/pp_directory_entry.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/array_writer.h"
#include "ppapi/shared_impl/file_ref_util.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_file_system_api.h"

namespace ppapi {
namespace proxy {

FileRefResource::FileRefResource(
    Connection connection,
    PP_Instance instance,
    const FileRef_CreateInfo& create_info)
    : PluginResource(connection, instance),
      create_info_(create_info),
      file_system_resource_(create_info.file_system_plugin_resource) {
  if (create_info_.file_system_type != PP_FILESYSTEMTYPE_EXTERNAL) {
    // If path ends with a slash, then normalize it away unless path is
    // the root path.
    int path_size = create_info_.internal_path.size();
    if (path_size > 1 && create_info_.internal_path.at(path_size - 1) == '/')
      create_info_.internal_path.erase(path_size - 1, 1);

    path_var_ = new StringVar(create_info_.internal_path);

    create_info_.display_name = GetNameForInternalFilePath(
        create_info_.internal_path);
  }
  name_var_ = new StringVar(create_info_.display_name);

  if (create_info_.pending_host_resource_id != 0) {
    AttachToPendingHost(BROWSER, create_info_.pending_host_resource_id);
  } else {
    CHECK(create_info_.file_system_type != PP_FILESYSTEMTYPE_EXTERNAL);
    SendCreate(BROWSER, PpapiHostMsg_FileRef_CreateInternal(
        create_info.file_system_plugin_resource,
        create_info.internal_path));
  }
}

FileRefResource::~FileRefResource() {
}

//static
PP_Resource FileRefResource::CreateFileRef(
    Connection connection,
    PP_Instance instance,
    const FileRef_CreateInfo& create_info) {
  // If we have a valid file_system resource, ensure that its type matches that
  // of the fs_type parameter.
  if (create_info.pending_host_resource_id != 0) {
    thunk::EnterResourceNoLock<thunk::PPB_FileSystem_API> enter(
        create_info.pending_host_resource_id, true);
    if (enter.failed())
      return 0;
    if (enter.object()->GetType() != create_info.file_system_type) {
      NOTREACHED() << "file system type mismatch with resource";
      return 0;
    }
  }

  if (create_info.file_system_type == PP_FILESYSTEMTYPE_LOCALPERSISTENT ||
      create_info.file_system_type == PP_FILESYSTEMTYPE_LOCALTEMPORARY) {
    if (!IsValidInternalPath(create_info.internal_path))
      return 0;
  }
  return (new FileRefResource(connection,
                              instance,
                              create_info))->GetReference();
}

thunk::PPB_FileRef_API* FileRefResource::AsPPB_FileRef_API() {
  // TODO: return "this" once we update PPB_FileRef_API.
  NOTREACHED();
  return NULL;
}

PP_FileSystemType FileRefResource::GetFileSystemType() const {
  return create_info_.file_system_type;
}

PP_Var FileRefResource::GetName() const {
  return name_var_->GetPPVar();
}

PP_Var FileRefResource::GetPath() const {
  if (create_info_.file_system_type == PP_FILESYSTEMTYPE_EXTERNAL)
    return PP_MakeUndefined();
  return path_var_->GetPPVar();
}

PP_Resource FileRefResource::GetParent() {
  if (create_info_.file_system_type == PP_FILESYSTEMTYPE_EXTERNAL)
    return 0;

  size_t pos = create_info_.internal_path.rfind('/');
  CHECK(pos != std::string::npos);
  if (pos == 0)
    pos++;
  std::string parent_path = create_info_.internal_path.substr(0, pos);

  ppapi::FileRef_CreateInfo parent_info;
  parent_info.file_system_type = create_info_.file_system_type;
  parent_info.internal_path = parent_path;
  parent_info.display_name = GetNameForInternalFilePath(parent_path);
  parent_info.file_system_plugin_resource =
      create_info_.file_system_plugin_resource;

  return (new FileRefResource(connection(),
                              pp_instance(),
                              parent_info))->GetReference();
}

int32_t FileRefResource::MakeDirectory(
    PP_Bool make_ancestors,
    scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_FileRef_MakeDirectoryReply>(
      BROWSER,
      PpapiHostMsg_FileRef_MakeDirectory(PP_TRUE == make_ancestors),
      base::Bind(&FileRefResource::RunTrackedCallback, this, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FileRefResource::Touch(PP_Time last_access_time,
                               PP_Time last_modified_time,
                               scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_FileRef_TouchReply>(
      BROWSER,
      PpapiHostMsg_FileRef_Touch(last_access_time,
                                 last_modified_time),
      base::Bind(&FileRefResource::RunTrackedCallback, this, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FileRefResource::Delete(scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_FileRef_DeleteReply>(
      BROWSER,
      PpapiHostMsg_FileRef_Delete(),
      base::Bind(&FileRefResource::RunTrackedCallback, this, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FileRefResource::Rename(PP_Resource new_file_ref,
                                scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_FileRef_RenameReply>(
      BROWSER,
      PpapiHostMsg_FileRef_Rename(new_file_ref),
      base::Bind(&FileRefResource::RunTrackedCallback, this, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FileRefResource::Query(PP_FileInfo* info,
                               scoped_refptr<TrackedCallback> callback) {
  if (info == NULL)
    return PP_ERROR_BADARGUMENT;

  Call<PpapiPluginMsg_FileRef_QueryReply>(
      BROWSER,
      PpapiHostMsg_FileRef_Query(),
      base::Bind(&FileRefResource::OnQueryReply, this, info, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FileRefResource::ReadDirectoryEntries(
    const PP_ArrayOutput& output,
    scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_FileRef_ReadDirectoryEntriesReply>(
      BROWSER,
      PpapiHostMsg_FileRef_ReadDirectoryEntries(),
      base::Bind(&FileRefResource::OnDirectoryEntriesReply,
                 this, output, callback));
  return PP_OK_COMPLETIONPENDING;
}

/*
const FileRef_CreateInfo& FileRefResource::GetCreateInfo() const {
  return create_info_;
}
*/
const PPB_FileRef_CreateInfo& FileRefResource::GetCreateInfo() const {
  // FIXME
  NOTREACHED();
  PPB_FileRef_CreateInfo *info = new PPB_FileRef_CreateInfo();
  return *info;
}

// TODO(teravest): Remove this when we are finished moving to the new proxy.
int32_t FileRefResource::QueryInHost(linked_ptr<PP_FileInfo> info,
                                    scoped_refptr<TrackedCallback> callback) {
  NOTREACHED();
  return PP_ERROR_FAILED;
}

// TODO(teravest): Remove this when we are finished moving to the new proxy.
int32_t FileRefResource::ReadDirectoryEntriesInHost(
    linked_ptr<std::vector<ppapi::PPB_FileRef_CreateInfo> > files,
    linked_ptr<std::vector<PP_FileType> > file_types,
    scoped_refptr<TrackedCallback> callback) {
  NOTREACHED();
  return PP_ERROR_FAILED;
}

PP_Var FileRefResource::GetAbsolutePath() {
  if (!absolute_path_var_.get()) {
    std::string absolute_path;
    int32_t result = SyncCall<PpapiPluginMsg_FileRef_GetAbsolutePathReply>(
        BROWSER, PpapiHostMsg_FileRef_GetAbsolutePath(), &absolute_path);
    if (result != PP_OK)
      return PP_MakeUndefined();
    absolute_path_var_ = new StringVar(absolute_path);
  }
  return absolute_path_var_->GetPPVar();
}

void FileRefResource::RunTrackedCallback(
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params) {
  if (TrackedCallback::IsPending(callback))
    callback->Run(params.result());
}

void FileRefResource::OnQueryReply(
    PP_FileInfo* out_info,
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params,
    const PP_FileInfo& info) {
  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK)
    *out_info = info;
  callback->Run(params.result());
}

void FileRefResource::OnDirectoryEntriesReply(
    const PP_ArrayOutput& output,
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params,
    const std::vector<ppapi::FileRef_CreateInfo>& infos,
    const std::vector<PP_FileType>& file_types) {
  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK) {
    ArrayWriter writer(output);
    if (!writer.is_valid()) {
      callback->Run(PP_ERROR_BADARGUMENT);
      return;
    }

    std::vector<PP_DirectoryEntry> entries;
    for (size_t i = 0; i < infos.size(); ++i) {
      PP_DirectoryEntry entry;
      entry.file_ref = FileRefResource::CreateFileRef(connection(),
                                                      pp_instance(),
                                                      infos[i]);
      entry.file_type = file_types[i];
      entries.push_back(entry);
    }

    writer.StoreVector(entries);
  }
  callback->Run(params.result());
}

}  // namespace proxy
}  // namespace ppapi
