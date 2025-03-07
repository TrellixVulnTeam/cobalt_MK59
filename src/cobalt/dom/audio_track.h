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

#ifndef COBALT_DOM_AUDIO_TRACK_H_
#define COBALT_DOM_AUDIO_TRACK_H_

#include <string>

#include "cobalt/dom/track_base.h"
#include "cobalt/script/wrappable.h"

namespace cobalt {
namespace dom {

class SourceBuffer;

// The AudioTrack interface holds attributes of an audio track.
//   https://www.w3.org/TR/html51/semantics-embedded-content.html#audiotrack-audiotrack
class AudioTrack : public TrackBase {
 public:
  // Custom, not in any spec.
  //
  AudioTrack(const std::string& id, const std::string& kind,
             const std::string& label, const std::string& language,
             bool enabled)
      : TrackBase(id, kind, label, language, &AudioTrack::IsValidKind),
        enabled_(enabled) {}

  // Web API: AudioTrack
  //
  using TrackBase::id;
  using TrackBase::kind;
  using TrackBase::label;
  using TrackBase::language;
  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) {
    if (enabled_ == enabled) {
      return;
    }
    enabled_ = enabled;
    if (media_element_) {
      // TODO: Notify the media element.
      // media_element_->OnAudioTrackChanged(this);
      NOTREACHED();
    }
  }

  // Custom, not in any spec.
  //
  static bool IsValidKind(const char* kind) {
    // https://www.w3.org/TR/html51/semantics-embedded-content.html#dom-audiotrack-audiotrackkind
    return SbStringCompareAll(kind, "alternative") == 0 ||
           SbStringCompareAll(kind, "descriptions") == 0 ||
           SbStringCompareAll(kind, "main") == 0 ||
           SbStringCompareAll(kind, "main-desc") == 0 ||
           SbStringCompareAll(kind, "translation") == 0 ||
           SbStringCompareAll(kind, "commentary") == 0 ||
           SbStringGetLength(kind) == 0;
  }

  DEFINE_WRAPPABLE_TYPE(AudioTrack);

 private:
  bool enabled_;
};

}  // namespace dom
}  // namespace cobalt

#endif  // COBALT_DOM_AUDIO_TRACK_H_
