// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPB_GRAPHICS_3D_PROXY_H_
#define PPAPI_PROXY_PPB_GRAPHICS_3D_PROXY_H_

#include <vector>

#include "base/memory/shared_memory.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "ppapi/c/pp_graphics_3d.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/proxy/interface_proxy.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/proxy/proxy_completion_callback_factory.h"
#include "ppapi/shared_impl/ppb_graphics_3d_shared.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace gpu {
struct Capabilities;
}

namespace ppapi {

class HostResource;

namespace proxy {

class SerializedHandle;
class PpapiCommandBufferProxy;

class PPAPI_PROXY_EXPORT Graphics3D : public PPB_Graphics3D_Shared {
 public:
  explicit Graphics3D(const HostResource& resource);
  ~Graphics3D() override;

  bool Init(gpu::gles2::GLES2Implementation* share_gles2,
            const gpu::Capabilities& capabilities,
            const SerializedHandle& shared_state);

  // Graphics3DTrusted API. These are not implemented in the proxy.
  PP_Bool SetGetBuffer(int32_t shm_id) override;
  PP_Bool Flush(int32_t put_offset) override;
  scoped_refptr<gpu::Buffer> CreateTransferBuffer(uint32_t size,
                                                  int32* id) override;
  PP_Bool DestroyTransferBuffer(int32_t id) override;
  gpu::CommandBuffer::State WaitForTokenInRange(int32_t start,
                                                int32_t end) override;
  gpu::CommandBuffer::State WaitForGetOffsetInRange(int32_t start,
                                                    int32_t end) override;
  uint32_t InsertSyncPoint() override;
  uint32_t InsertFutureSyncPoint() override;
  void RetireSyncPoint(uint32_t sync_point) override;

 private:
  // PPB_Graphics3D_Shared overrides.
  gpu::CommandBuffer* GetCommandBuffer() override;
  gpu::GpuControl* GetGpuControl() override;
  int32 DoSwapBuffers() override;

  scoped_ptr<PpapiCommandBufferProxy> command_buffer_;

  DISALLOW_COPY_AND_ASSIGN(Graphics3D);
};

class PPB_Graphics3D_Proxy : public InterfaceProxy {
 public:
  PPB_Graphics3D_Proxy(Dispatcher* dispatcher);
  ~PPB_Graphics3D_Proxy();

  static PP_Resource CreateProxyResource(
      PP_Instance instance,
      PP_Resource share_context,
      const int32_t* attrib_list);

  // InterfaceProxy implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;

  static const ApiID kApiID = API_ID_PPB_GRAPHICS_3D;

 private:
  void OnMsgCreate(PP_Instance instance,
                   HostResource share_context,
                   const std::vector<int32_t>& attribs,
                   HostResource* result,
                   gpu::Capabilities* capabilities,
                   SerializedHandle* handle);
  void OnMsgSetGetBuffer(const HostResource& context,
                         int32 id);
  void OnMsgWaitForTokenInRange(const HostResource& context,
                                int32 start,
                                int32 end,
                                gpu::CommandBuffer::State* state,
                                bool* success);
  void OnMsgWaitForGetOffsetInRange(const HostResource& context,
                                    int32 start,
                                    int32 end,
                                    gpu::CommandBuffer::State* state,
                                    bool* success);
  void OnMsgAsyncFlush(const HostResource& context, int32 put_offset);
  void OnMsgCreateTransferBuffer(
      const HostResource& context,
      uint32 size,
      int32* id,
      ppapi::proxy::SerializedHandle* transfer_buffer);
  void OnMsgDestroyTransferBuffer(const HostResource& context,
                                  int32 id);
  void OnMsgSwapBuffers(const HostResource& context);
  void OnMsgInsertSyncPoint(const HostResource& context, uint32* sync_point);
  void OnMsgInsertFutureSyncPoint(const HostResource& context,
                                  uint32* sync_point);
  void OnMsgRetireSyncPoint(const HostResource& context, uint32 sync_point);
  // Renderer->plugin message handlers.
  void OnMsgSwapBuffersACK(const HostResource& context,
                           int32_t pp_error);

  void SendSwapBuffersACKToPlugin(int32_t result,
                                  const HostResource& context);

  ProxyCompletionCallbackFactory<PPB_Graphics3D_Proxy> callback_factory_;

  DISALLOW_COPY_AND_ASSIGN(PPB_Graphics3D_Proxy);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_PPB_GRAPHICS_3D_PROXY_H_

