// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_PPAPI_FEATURES_H_
#define PPAPI_SHARED_IMPL_PPAPI_FEATURES_H_

#include "base/feature_list.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"

namespace ppapi {
namespace features {

PPAPI_SHARED_EXPORT extern const base::Feature kStreamToFile;

}  // namespace features
}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_PPAPI_FEATURES_H_
