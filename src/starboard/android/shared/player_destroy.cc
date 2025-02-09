// Copyright 2017 The Cobalt Authors. All Rights Reserved.
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

#include "starboard/player.h"

#include "starboard/android/shared/cobalt/android_media_session_client.h"
#include "starboard/shared/starboard/player/player_internal.h"

using starboard::android::shared::cobalt::kNone;
using starboard::android::shared::cobalt::
    UpdateActiveSessionPlatformPlaybackState;
using starboard::android::shared::cobalt::UpdateActiveSessionPlatformPlayer;

void SbPlayerDestroy(SbPlayer player) {
  if (!SbPlayerIsValid(player)) {
    return;
  }
  UpdateActiveSessionPlatformPlaybackState(kNone);
  UpdateActiveSessionPlatformPlayer(kSbPlayerInvalid);
  delete player;
}
