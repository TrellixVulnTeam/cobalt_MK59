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

#include "starboard/shared/starboard/player/filter/player_components.h"

#include "starboard/common/ref_counted.h"
#include "starboard/common/scoped_ptr.h"
#include "starboard/media.h"
#include "starboard/shared/ffmpeg/ffmpeg_audio_decoder.h"
#include "starboard/shared/ffmpeg/ffmpeg_video_decoder.h"
#include "starboard/shared/libaom/aom_video_decoder.h"
#include "starboard/shared/libvpx/vpx_video_decoder.h"
#include "starboard/shared/starboard/player/filter/audio_decoder_internal.h"
#include "starboard/shared/starboard/player/filter/audio_renderer_sink.h"
#include "starboard/shared/starboard/player/filter/audio_renderer_sink_impl.h"
#include "starboard/shared/starboard/player/filter/punchout_video_renderer_sink.h"
#include "starboard/shared/starboard/player/filter/video_decoder_internal.h"
#include "starboard/shared/starboard/player/filter/video_render_algorithm.h"
#include "starboard/shared/starboard/player/filter/video_render_algorithm_impl.h"
#include "starboard/shared/starboard/player/filter/video_renderer_sink.h"

namespace starboard {
namespace shared {
namespace starboard {
namespace player {
namespace filter {

namespace {

class PlayerComponentsImpl : public PlayerComponents {
  void CreateAudioComponents(
      const AudioParameters& audio_parameters,
      scoped_ptr<AudioDecoder>* audio_decoder,
      scoped_ptr<AudioRendererSink>* audio_renderer_sink) override {
    typedef ::starboard::shared::ffmpeg::AudioDecoder AudioDecoderImpl;

    SB_DCHECK(audio_decoder);
    SB_DCHECK(audio_renderer_sink);

    scoped_ptr<AudioDecoderImpl> audio_decoder_impl(AudioDecoderImpl::Create(
        audio_parameters.audio_codec, audio_parameters.audio_header));
    if (audio_decoder_impl && audio_decoder_impl->is_valid()) {
      audio_decoder->reset(audio_decoder_impl.release());
    } else {
      audio_decoder->reset();
    }
    audio_renderer_sink->reset(new AudioRendererSinkImpl);
  }

  void CreateVideoComponents(
      const VideoParameters& video_parameters,
      scoped_ptr<VideoDecoder>* video_decoder,
      scoped_ptr<VideoRenderAlgorithm>* video_render_algorithm,
      scoped_refptr<VideoRendererSink>* video_renderer_sink) override {
    typedef ::starboard::shared::aom::VideoDecoder Av1VideoDecoderImpl;
    typedef ::starboard::shared::ffmpeg::VideoDecoder FfmpegVideoDecoderImpl;
    typedef ::starboard::shared::vpx::VideoDecoder VpxVideoDecoderImpl;

    const SbTime kVideoSinkRenderInterval = 10 * kSbTimeMillisecond;

    SB_DCHECK(video_decoder);
    SB_DCHECK(video_render_algorithm);
    SB_DCHECK(video_renderer_sink);

    video_decoder->reset();

    if (video_parameters.video_codec == kSbMediaVideoCodecVp9) {
      video_decoder->reset(new VpxVideoDecoderImpl(
          video_parameters.video_codec, video_parameters.output_mode,
          video_parameters.decode_target_graphics_context_provider));
#if SB_API_VERSION < SB_HAS_AV1_VERSION
    } else if (video_parameters.video_codec == kSbMediaVideoCodecVp10) {
#else   // SB_API_VERSION < SB_HAS_AV1_VERSION
    } else if (video_parameters.video_codec == kSbMediaVideoCodecAv1) {
#endif  // SB_API_VERSION < SB_HAS_AV1_VERSION
      video_decoder->reset(new Av1VideoDecoderImpl(
          video_parameters.video_codec, video_parameters.output_mode,
          video_parameters.decode_target_graphics_context_provider));
    } else {
      scoped_ptr<FfmpegVideoDecoderImpl> ffmpeg_video_decoder(
          FfmpegVideoDecoderImpl::Create(
              video_parameters.video_codec, video_parameters.output_mode,
              video_parameters.decode_target_graphics_context_provider));
      if (ffmpeg_video_decoder && ffmpeg_video_decoder->is_valid()) {
        video_decoder->reset(ffmpeg_video_decoder.release());
      }
    }

    video_render_algorithm->reset(new VideoRenderAlgorithmImpl);
    if (video_parameters.output_mode == kSbPlayerOutputModeDecodeToTexture) {
      *video_renderer_sink = NULL;
    } else {
      *video_renderer_sink = new PunchoutVideoRendererSink(
          video_parameters.player, kVideoSinkRenderInterval);
    }
  }

  void GetAudioRendererParams(int* max_cached_frames,
                              int* max_frames_per_append) const override {
    SB_DCHECK(max_cached_frames);
    SB_DCHECK(max_frames_per_append);

    *max_cached_frames = 256 * 1024;
    *max_frames_per_append = 16384;
  }
};

}  // namespace

// static
scoped_ptr<PlayerComponents> PlayerComponents::Create() {
  return make_scoped_ptr<PlayerComponents>(new PlayerComponentsImpl);
}

}  // namespace filter
}  // namespace player
}  // namespace starboard
}  // namespace shared
}  // namespace starboard
