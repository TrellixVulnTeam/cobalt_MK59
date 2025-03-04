// Copyright 2018 Google Inc. All Rights Reserved.
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
#ifndef COBALT_MEDIA_CAPTURE_ENCODERS_AUDIO_ENCODER_H_
#define COBALT_MEDIA_CAPTURE_ENCODERS_AUDIO_ENCODER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/string_piece.h"
#include "base/time.h"
#include "cobalt/media_stream/audio_parameters.h"
#include "starboard/mutex.h"

#if defined(COBALT_MEDIA_SOURCE_2016)
#include "cobalt/media/base/shell_audio_bus.h"
#else  // defined(COBALT_MEDIA_SOURCE_2016)
#include "media/base/shell_audio_bus.h"
#endif  // defined(COBALT_MEDIA_SOURCE_2016)

namespace cobalt {
namespace media_capture {
namespace encoders {

class AudioEncoder {
 public:
#if defined(COBALT_MEDIA_SOURCE_2016)
  typedef media::ShellAudioBus ShellAudioBus;
#else   // defined(COBALT_MEDIA_SOURCE_2016)
  typedef ::media::ShellAudioBus ShellAudioBus;
#endif  // defined(COBALT_MEDIA_SOURCE_2016)

  class Listener {
   public:
    virtual ~Listener() = default;
    virtual void OnEncodedDataAvailable(const uint8* data, size_t data_size,
                                        base::TimeTicks timecode) = 0;
  };

  AudioEncoder() = default;
  virtual ~AudioEncoder() = default;

  // Encode raw audio data.
  virtual void Encode(const ShellAudioBus& audio_bus,
                      base::TimeTicks reference_time) = 0;
  // Finish encoding.
  virtual void Finish(base::TimeTicks timecode) = 0;

  virtual std::string GetMimeType() const = 0;

  virtual void OnSetFormat(const media_stream::AudioParameters& params) = 0;

  void AddListener(AudioEncoder::Listener* listener);

  void RemoveListener(AudioEncoder::Listener* listener);

  // Returns the estimated bitrate for a stream with that has
  // same properties as |params|.  Returns a positive number on success,
  // and non-positive number on failure.  The return value is only
  // a best guess, and the encoder is free to output at a lower or higher
  // bitrate.
  virtual int64 GetEstimatedOutputBitsPerSecond(
      const media_stream::AudioParameters& params) const = 0;

 protected:
  // This call is thread-safe.
  void PushDataToAllListeners(const uint8* data, size_t data_size,
                              base::TimeTicks timecode);

 private:
  AudioEncoder(const AudioEncoder&) = delete;
  AudioEncoder& operator=(AudioEncoder&) = delete;

  starboard::Mutex listeners_mutex_;
  std::vector<AudioEncoder::Listener*> listeners_;
};

}  // namespace encoders
}  // namespace media_capture
}  // namespace cobalt

#endif  // COBALT_MEDIA_CAPTURE_ENCODERS_AUDIO_ENCODER_H_
