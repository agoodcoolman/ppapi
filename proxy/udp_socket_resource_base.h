// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_UDP_SOCKET_SHARED_H_
#define PPAPI_PROXY_UDP_SOCKET_SHARED_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/c/private/ppb_udp_socket_private.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/tracked_callback.h"

namespace ppapi {
namespace proxy {

class ResourceMessageReplyParams;

class PPAPI_PROXY_EXPORT UDPSocketResourceBase: public PluginResource {
 public:
  // The maximum number of bytes that each PpapiHostMsg_PPBUDPSocket_RecvFrom
  // message is allowed to request.
  static const int32_t kMaxReadSize;
  // The maximum number of bytes that each PpapiHostMsg_PPBUDPSocket_SendTo
  // message is allowed to carry.
  static const int32_t kMaxWriteSize;

 protected:
  UDPSocketResourceBase(Connection connection, PP_Instance instance);
  virtual ~UDPSocketResourceBase();

  int32_t SetSocketFeatureImpl(PP_UDPSocketFeature_Private name,
                               const PP_Var& value);
  int32_t BindImpl(const PP_NetAddress_Private* addr,
                   scoped_refptr<TrackedCallback> callback);
  PP_Bool GetBoundAddressImpl(PP_NetAddress_Private* addr);
  // |addr| could be NULL to indicate that an output value is not needed.
  int32_t RecvFromImpl(char* buffer,
                       int32_t num_bytes,
                       PP_Resource* addr,
                       scoped_refptr<TrackedCallback> callback);
  PP_Bool GetRecvFromAddressImpl(PP_NetAddress_Private* addr);
  int32_t SendToImpl(const char* buffer,
                     int32_t num_bytes,
                     const PP_NetAddress_Private* addr,
                     scoped_refptr<TrackedCallback> callback);
  void CloseImpl();

 private:
  void PostAbortIfNecessary(scoped_refptr<TrackedCallback>* callback);

  // IPC message handlers.
  void OnPluginMsgBindReply(const ResourceMessageReplyParams& params,
                            const PP_NetAddress_Private& bound_addr);
  void OnPluginMsgRecvFromReply(PP_Resource* output_addr,
                                const ResourceMessageReplyParams& params,
                                const std::string& data,
                                const PP_NetAddress_Private& addr);
  void OnPluginMsgSendToReply(const ResourceMessageReplyParams& params,
                              int32_t bytes_written);

  bool bound_;
  bool closed_;

  scoped_refptr<TrackedCallback> bind_callback_;
  scoped_refptr<TrackedCallback> recvfrom_callback_;
  scoped_refptr<TrackedCallback> sendto_callback_;

  char* read_buffer_;
  int32_t bytes_to_read_;

  PP_NetAddress_Private recvfrom_addr_;
  PP_NetAddress_Private bound_addr_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketResourceBase);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_UDP_SOCKET_SHARED_H_
