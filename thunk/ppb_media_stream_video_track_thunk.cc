// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From ppb_media_stream_video_track.idl modified Wed Jan  8 14:08:11 2014.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_media_stream_video_track.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/ppb_media_stream_video_track_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsMediaStreamVideoTrack(PP_Resource resource) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::IsMediaStreamVideoTrack()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Configure(PP_Resource video_track, uint32_t max_buffered_frames) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::Configure()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->Configure(max_buffered_frames);
}

struct PP_Var GetId(PP_Resource video_track) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::GetId()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track, true);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.object()->GetId();
}

PP_Bool HasEnded(PP_Resource video_track) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::HasEnded()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track, true);
  if (enter.failed())
    return PP_TRUE;
  return enter.object()->HasEnded();
}

int32_t GetFrame(PP_Resource video_track,
                 PP_Resource* frame,
                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::GetFrame()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track,
                                                     callback,
                                                     true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->GetFrame(frame, enter.callback()));
}

int32_t RecycleFrame(PP_Resource video_track, PP_Resource frame) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::RecycleFrame()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->RecycleFrame(frame);
}

void Close(PP_Resource video_track) {
  VLOG(4) << "PPB_MediaStreamVideoTrack::Close()";
  EnterResource<PPB_MediaStreamVideoTrack_API> enter(video_track, true);
  if (enter.failed())
    return;
  enter.object()->Close();
}

const PPB_MediaStreamVideoTrack_0_1 g_ppb_mediastreamvideotrack_thunk_0_1 = {
  &IsMediaStreamVideoTrack,
  &Configure,
  &GetId,
  &HasEnded,
  &GetFrame,
  &RecycleFrame,
  &Close
};

}  // namespace

const PPB_MediaStreamVideoTrack_0_1* GetPPB_MediaStreamVideoTrack_0_1_Thunk() {
  return &g_ppb_mediastreamvideotrack_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
