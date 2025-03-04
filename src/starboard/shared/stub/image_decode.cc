// Copyright 2016 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/configuration.h"
#include "starboard/image.h"

#if !SB_HAS(GRAPHICS)
#error "SbImageDecode requires SB_HAS(GRAPHICS)."
#endif

SbDecodeTarget SbImageDecode(SbDecodeTargetGraphicsContextProvider* provider,
                             void* data,
                             int data_size,
                             const char* mime_type,
                             SbDecodeTargetFormat format) {
  SB_UNREFERENCED_PARAMETER(data);
  SB_UNREFERENCED_PARAMETER(data_size);
  SB_UNREFERENCED_PARAMETER(format);
  SB_UNREFERENCED_PARAMETER(mime_type);
  SB_UNREFERENCED_PARAMETER(provider);
  return kSbDecodeTargetInvalid;
}
