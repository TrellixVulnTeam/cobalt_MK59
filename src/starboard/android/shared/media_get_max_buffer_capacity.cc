// Copyright 2018 The Cobalt Authors. All Rights Reserved.
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

#include "starboard/media.h"

int SbMediaGetMaxBufferCapacity(SbMediaVideoCodec codec,
                                int resolution_width,
                                int resolution_height,
                                int bits_per_pixel) {
  SB_UNREFERENCED_PARAMETER(codec);
  SB_UNREFERENCED_PARAMETER(resolution_width);
  SB_UNREFERENCED_PARAMETER(resolution_height);
  SB_UNREFERENCED_PARAMETER(bits_per_pixel);
  // TODO: refine this to a more reasonable value, taking into account
  // resolution. On most platforms this is 36 * 1024 * 1024 for 1080p, and
  // 65 * 1024 * 1024 for 4k.
  return 500 * 1024 * 1024;
}
