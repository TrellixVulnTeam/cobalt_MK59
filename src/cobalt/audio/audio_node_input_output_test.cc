// Copyright 2015 The Cobalt Authors. All Rights Reserved.
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

#include <math.h>

#include "cobalt/audio/audio_buffer_source_node.h"
#include "cobalt/audio/audio_context.h"
#include "cobalt/audio/audio_helpers.h"
#include "cobalt/script/global_environment.h"
#include "cobalt/script/javascript_engine.h"
#include "cobalt/script/typed_arrays.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO: Consolidate ShellAudioBus creation code

namespace cobalt {
namespace audio {

#if defined(COBALT_MEDIA_SOURCE_2016)
typedef media::ShellAudioBus ShellAudioBus;
#else   // defined(COBALT_MEDIA_SOURCE_2016)
typedef ::media::ShellAudioBus ShellAudioBus;
#endif  // defined(COBALT_MEDIA_SOURCE_2016)

constexpr int kRenderBufferSizeFrames = 32;

class AudioDestinationNodeMock : public AudioNode,
                                 public AudioDevice::RenderCallback {
#if defined(COBALT_MEDIA_SOURCE_2016)
  typedef media::ShellAudioBus ShellAudioBus;
#else   // defined(COBALT_MEDIA_SOURCE_2016)
  typedef ::media::ShellAudioBus ShellAudioBus;
#endif  // defined(COBALT_MEDIA_SOURCE_2016)

 public:
  explicit AudioDestinationNodeMock(AudioContext* context)
      : AudioNode(context) {
    AudioLock::AutoLock lock(audio_lock());

    AddInput(new AudioNodeInput(this));
  }

  // From AudioNode.
  scoped_ptr<ShellAudioBus> PassAudioBusFromSource(int32, /*number_of_frames*/
                                                   SampleType, bool*) override {
    NOTREACHED();
    return scoped_ptr<ShellAudioBus>();
  }

  // From AudioDevice::RenderCallback.
  void FillAudioBus(bool all_consumed, ShellAudioBus* audio_bus,
                    bool* silence) override {
    UNREFERENCED_PARAMETER(all_consumed);

    AudioLock::AutoLock lock(audio_lock());

    bool all_finished;
    // Destination node only has one input.
    Input(0)->FillAudioBus(audio_bus, silence, &all_finished);
  }
};

void FillAudioBusFromOneSource(
    scoped_ptr<ShellAudioBus> src_data,
    const AudioNodeChannelInterpretation& interpretation,
    ShellAudioBus* audio_bus, bool* silence) {
  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_refptr<AudioBufferSourceNode> source(
      audio_context->CreateBufferSource());
  scoped_refptr<AudioBuffer> buffer(
      new AudioBuffer(audio_context->sample_rate(), src_data.Pass()));
  source->set_buffer(buffer);

  scoped_refptr<AudioDestinationNodeMock> destination(
      new AudioDestinationNodeMock(audio_context.get()));
  destination->set_channel_interpretation(interpretation);
  source->Connect(destination, 0, 0, NULL);
  source->Start(0, 0, NULL);

  destination->FillAudioBus(true, audio_bus, silence);
}

class AudioNodeInputOutputTest : public ::testing::Test {
 public:
  AudioNodeInputOutputTest()
      : engine_(script::JavaScriptEngine::CreateEngine()),
        global_environment_(engine_->CreateGlobalEnvironment()) {
    global_environment_->CreateGlobalObject();
  }

  ~AudioNodeInputOutputTest() {
    global_environment_->SetReportEvalCallback(base::Closure());
    global_environment_->SetReportErrorCallback(
        script::GlobalEnvironment::ReportErrorCallback());
    global_environment_ = nullptr;
  }

  script::GlobalEnvironment* global_environment() const {
    return global_environment_.get();
  }

 private:
  scoped_ptr<script::JavaScriptEngine> engine_;
  scoped_refptr<script::GlobalEnvironment> global_environment_;

 protected:
  MessageLoop message_loop_;
};

TEST_F(AudioNodeInputOutputTest, StereoToStereoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[50];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          20.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));
  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 40);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, StereoToStereoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[50];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          20.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 40);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, MonoToStereoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 1;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[25];
  for (size_t i = 0; i < kNumOfFrames; ++i) {
    src_data_in_float[i] = 50.0f;
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 50);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, MonoToStereoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 1;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[25];
  for (size_t i = 0; i < kNumOfFrames; ++i) {
    src_data_in_float[i] = 50.0f;
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames && channel == 0) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 50);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, QuadToStereoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 4;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[100];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 30);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, QuadToStereoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 4;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[100];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 10);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, FivePointOneToStereoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 6;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 10;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[60];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_FLOAT_EQ(audio_bus->GetFloat32Sample(channel, frame), 66.568f);
        } else {
          EXPECT_FLOAT_EQ(audio_bus->GetFloat32Sample(channel, frame), 83.639f);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, FivePointOneToStereoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 6;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfFrames = 10;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[60];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 10);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, StereoToMonoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[50];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          20.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 30);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, StereoToMonoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[50];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          20.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 20);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, QuadToMonoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 4;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[100];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 25);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, QuadToMonoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 4;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 25;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[100];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 10);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, FivePointOneToMonoSpeakersLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 6;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 10;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  float src_data_in_float[60];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_FLOAT_EQ(audio_bus->GetFloat32Sample(channel, frame), 106.213f);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, FivePointOneToMonoDiscreteLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 6;
  constexpr size_t kNumOfDestChannels = 1;
  constexpr size_t kNumOfFrames = 10;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationDiscrete;

  float src_data_in_float[60];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_data_in_float[frame * kNumOfSrcChannels + channel] =
          10.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data(
      new ShellAudioBus(kNumOfSrcChannels, kNumOfFrames, src_data_in_float));

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  FillAudioBusFromOneSource(src_data.Pass(), kInterpretation, audio_bus.get(),
                            &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames) {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 10);
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, MultipleInputNodesLayoutTest) {
  scoped_refptr<AudioContext> audio_context(new AudioContext());

  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 2;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;

  constexpr size_t kNumOfFrames_1 = 25;
  float src_data_in_float_1[50];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames_1; ++frame) {
      src_data_in_float_1[frame * kNumOfSrcChannels + channel] =
          20.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data_1(new ShellAudioBus(
      kNumOfSrcChannels, kNumOfFrames_1, src_data_in_float_1));
  scoped_refptr<AudioBufferSourceNode> source_1(
      audio_context->CreateBufferSource());
  scoped_refptr<AudioBuffer> buffer_1(
      new AudioBuffer(audio_context->sample_rate(), src_data_1.Pass()));
  source_1->set_buffer(buffer_1);

  constexpr size_t kNumOfFrames_2 = 50;
  float src_data_in_float_2[100];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames_2; ++frame) {
      src_data_in_float_2[frame * kNumOfSrcChannels + channel] =
          40.0f * (channel + 1);
    }
  }

  scoped_ptr<ShellAudioBus> src_data_2(new ShellAudioBus(
      kNumOfSrcChannels, kNumOfFrames_2, src_data_in_float_2));
  scoped_refptr<AudioBufferSourceNode> source_2(
      audio_context->CreateBufferSource());
  scoped_refptr<AudioBuffer> buffer_2(
      new AudioBuffer(audio_context->sample_rate(), src_data_2.Pass()));
  source_2->set_buffer(buffer_2);

  scoped_refptr<AudioDestinationNodeMock> destination(
      new AudioDestinationNodeMock(audio_context.get()));
  destination->set_channel_interpretation(kInterpretation);
  source_1->Connect(destination, 0, 0, NULL);
  source_2->Connect(destination, 0, 0, NULL);
  source_1->Start(0, 0, NULL);
  source_2->Start(0, 0, NULL);

  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfDestChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kPlanar));
  audio_bus->ZeroAllFrames();
  bool silence = true;
  destination->FillAudioBus(true, audio_bus.get(), &silence);
  EXPECT_FALSE(silence);

  for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames_1) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 60);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 120);
        }
      } else if (frame < kNumOfFrames_2) {
        if (channel == 0) {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 40);
        } else {
          EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 80);
        }
      } else {
        EXPECT_EQ(audio_bus->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, CreateBufferLayoutTest) {
  constexpr size_t kNumOfChannels = 2;
  constexpr size_t kNumOfFrames = 25;

  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_refptr<AudioBuffer> buffer(audio_context->CreateBuffer(
      kNumOfChannels, kNumOfFrames, audio_context->sample_rate()));

  EXPECT_EQ(buffer->number_of_channels(), kNumOfChannels);
  EXPECT_EQ(buffer->length(), kNumOfFrames);
  EXPECT_EQ(buffer->sample_rate(), audio_context->sample_rate());
}

TEST_F(AudioNodeInputOutputTest, CopyToChannelPlanarFloat32LayoutTest) {
  constexpr size_t kNumOfChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  constexpr size_t kOffset = 8;

  float src_arr[kNumOfChannels][kNumOfFrames];
  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_arr[channel][frame] = channel * kNumOfFrames + frame;
    }
  }

  script::Handle<script::Float32Array> channel0_arr =
      script::Float32Array::New(global_environment(), src_arr[0], kNumOfFrames);
  script::Handle<script::Float32Array> channel1_arr =
      script::Float32Array::New(global_environment(), src_arr[1], kNumOfFrames);

  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kPlanar));
  audio_bus->ZeroAllFrames();
  scoped_refptr<AudioBuffer> buffer(
      new AudioBuffer(audio_context->sample_rate(), audio_bus.Pass()));
  buffer->CopyToChannel(channel0_arr, 0, kOffset, NULL);
  buffer->CopyToChannel(channel1_arr, 1, kOffset, NULL);

  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames + kOffset && frame >= kOffset) {
        EXPECT_EQ(buffer->audio_bus()->GetFloat32Sample(channel, frame),
                  (channel * kNumOfFrames + (frame - kOffset)));
      } else {
        EXPECT_EQ(buffer->audio_bus()->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, CopyToChannelInterleavedFloat32LayoutTest) {
  constexpr size_t kNumOfChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  constexpr size_t kOffset = 8;

  float src_arr[kNumOfChannels][kNumOfFrames];
  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_arr[channel][frame] = channel * kNumOfFrames + frame;
    }
  }

  script::Handle<script::Float32Array> channel0_arr =
      script::Float32Array::New(global_environment(), src_arr[0], kNumOfFrames);
  script::Handle<script::Float32Array> channel1_arr =
      script::Float32Array::New(global_environment(), src_arr[1], kNumOfFrames);

  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kFloat32, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  scoped_refptr<AudioBuffer> buffer(
      new AudioBuffer(audio_context->sample_rate(), audio_bus.Pass()));
  buffer->CopyToChannel(channel0_arr, 0, kOffset, NULL);
  buffer->CopyToChannel(channel1_arr, 1, kOffset, NULL);

  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames + kOffset && frame >= kOffset) {
        EXPECT_EQ(buffer->audio_bus()->GetFloat32Sample(channel, frame),
                  (channel * kNumOfFrames + (frame - kOffset)));
      } else {
        EXPECT_EQ(buffer->audio_bus()->GetFloat32Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, CopyToChannelPlanarInt16LayoutTest) {
  constexpr size_t kNumOfChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  constexpr size_t kOffset = 8;

  float src_arr[kNumOfChannels][kNumOfFrames];
  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_arr[channel][frame] = 0.001f * (channel * kNumOfFrames + frame);
    }
  }

  script::Handle<script::Float32Array> channel0_arr =
      script::Float32Array::New(global_environment(), src_arr[0], kNumOfFrames);
  script::Handle<script::Float32Array> channel1_arr =
      script::Float32Array::New(global_environment(), src_arr[1], kNumOfFrames);

  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kInt16, ShellAudioBus::kPlanar));
  audio_bus->ZeroAllFrames();
  scoped_refptr<AudioBuffer> buffer(
      new AudioBuffer(audio_context->sample_rate(), audio_bus.Pass()));
  buffer->CopyToChannel(channel0_arr, 0, kOffset, NULL);
  buffer->CopyToChannel(channel1_arr, 1, kOffset, NULL);

  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames + kOffset && frame >= kOffset) {
        int16 expected_val = ConvertSample<float, int16>
            (0.001f * (channel * kNumOfFrames + (frame - kOffset)));
        EXPECT_EQ(buffer->audio_bus()->GetInt16Sample(channel, frame),
                  expected_val);
      } else {
        EXPECT_EQ(buffer->audio_bus()->GetInt16Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, CopyToChannelInterleavedInt16LayoutTest) {
  constexpr size_t kNumOfChannels = 2;
  constexpr size_t kNumOfFrames = 25;
  constexpr size_t kOffset = 8;

  float src_arr[kNumOfChannels][kNumOfFrames];
  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfFrames; ++frame) {
      src_arr[channel][frame] = 0.001f * (channel * kNumOfFrames + frame);
    }
  }

  script::Handle<script::Float32Array> channel0_arr =
      script::Float32Array::New(global_environment(), src_arr[0], kNumOfFrames);
  script::Handle<script::Float32Array> channel1_arr =
      script::Float32Array::New(global_environment(), src_arr[1], kNumOfFrames);

  scoped_refptr<AudioContext> audio_context(new AudioContext());
  scoped_ptr<ShellAudioBus> audio_bus(
      new ShellAudioBus(kNumOfChannels, kRenderBufferSizeFrames,
                        ShellAudioBus::kInt16, ShellAudioBus::kInterleaved));
  audio_bus->ZeroAllFrames();
  scoped_refptr<AudioBuffer> buffer(
      new AudioBuffer(audio_context->sample_rate(), audio_bus.Pass()));
  buffer->CopyToChannel(channel0_arr, 0, kOffset, NULL);
  buffer->CopyToChannel(channel1_arr, 1, kOffset, NULL);

  for (size_t channel = 0; channel < kNumOfChannels; ++channel) {
    for (size_t frame = 0; frame < kRenderBufferSizeFrames; ++frame) {
      if (frame < kNumOfFrames + kOffset && frame >= kOffset) {
        int16 expected_val = ConvertSample<float, int16>
            (0.001f * (channel * kNumOfFrames + (frame - kOffset)));
        EXPECT_EQ(buffer->audio_bus()->GetInt16Sample(channel, frame),
                  expected_val);
      } else {
        EXPECT_EQ(buffer->audio_bus()->GetInt16Sample(channel, frame), 0);
      }
    }
  }
}

TEST_F(AudioNodeInputOutputTest, ResampleBufferSampleRateLayoutTest) {
  constexpr size_t kNumOfSrcChannels = 2;
  constexpr size_t kNumOfDestChannels = 2;
  constexpr size_t kNumOfSrcFrames = 500;
  constexpr size_t kNumOfDestFrames = 640;
  const AudioNodeChannelInterpretation kInterpretation =
      kAudioNodeChannelInterpretationSpeakers;
  const size_t kBufferSampleRateArr[2] = {44100, 52200};
  const SampleType kSampleTypeArr[2] = {kSampleTypeFloat32, kSampleTypeInt16};
  constexpr float kMaxAvgError = 0.01f;

  float src_arr[kNumOfSrcChannels][kNumOfSrcFrames];
  for (size_t channel = 0; channel < kNumOfSrcChannels; ++channel) {
    for (size_t frame = 0; frame < kNumOfSrcFrames; ++frame) {
      // Values range from 0 to 1
      float func_arg = (channel + kNumOfSrcChannels * frame) /
                       static_cast<float>(kNumOfSrcFrames * kNumOfSrcChannels);
      // Values range from -1 to 1
      src_arr[channel][frame] = static_cast<float>(sin(2 * M_PI * func_arg));
    }
  }

  script::Handle<script::Float32Array> channel_0_arr =
      script::Float32Array::New(global_environment(), src_arr[0],
                                kNumOfSrcFrames);
  script::Handle<script::Float32Array> channel_1_arr =
      script::Float32Array::New(global_environment(), src_arr[1],
                                kNumOfSrcFrames);

  for (size_t buffer_sample_rate : kBufferSampleRateArr) {
    for (SampleType sample_type : kSampleTypeArr) {
      scoped_ptr<ShellAudioBus> src_data(
          new ShellAudioBus(kNumOfSrcChannels, kNumOfSrcFrames, sample_type,
                            ShellAudioBus::kInterleaved));
      src_data->ZeroAllFrames();
      scoped_refptr<AudioBuffer> buffer(
          new AudioBuffer(buffer_sample_rate, src_data.Pass()));
      buffer->CopyToChannel(channel_0_arr, 0, 0, NULL);
      buffer->CopyToChannel(channel_1_arr, 1, 0, NULL);

      scoped_refptr<AudioContext> audio_context(new AudioContext());
      scoped_refptr<AudioBufferSourceNode> source(
          audio_context->CreateBufferSource());
      source->set_buffer(buffer);

      scoped_refptr<AudioDestinationNodeMock> destination(
          new AudioDestinationNodeMock(audio_context.get()));
      destination->set_channel_interpretation(kInterpretation);
      source->Connect(destination, 0, 0, NULL);
      source->Start(0, 0, NULL);

      scoped_ptr<ShellAudioBus> audio_bus(
          new ShellAudioBus(kNumOfDestChannels, kNumOfDestFrames, sample_type,
                            ShellAudioBus::kInterleaved));
      audio_bus->ZeroAllFrames();
      bool silence = true;
      destination->FillAudioBus(true, audio_bus.get(), &silence);
      EXPECT_FALSE(silence);

      size_t num_output_frames = static_cast<size_t>(
          kNumOfSrcFrames * audio_context->sample_rate() / buffer_sample_rate);
      float sum_of_errors = 0;
      for (size_t channel = 0; channel < kNumOfDestChannels; ++channel) {
        for (size_t frame = 0; frame < kNumOfDestFrames; ++frame) {
          float func_arg =
              (channel + kNumOfSrcChannels * frame) /
              static_cast<float>((num_output_frames * kNumOfSrcChannels));
          if (sample_type == kSampleTypeFloat32) {
            float actual_val = audio_bus->GetFloat32Sample(channel, frame);
            float expected_val = 0.0f;
            if (frame < num_output_frames) {
              expected_val = static_cast<float>(sin(2 * M_PI * func_arg));
            }
            sum_of_errors += fabs(actual_val - expected_val);
          } else {
            int16 actual_val = audio_bus->GetInt16Sample(channel, frame);
            int16 expected_val = 0;
            if (frame < num_output_frames) {
              expected_val = ConvertSample<float, int16>(
                  static_cast<float>(sin(2 * M_PI * func_arg)));
            }
            sum_of_errors += ConvertSample<int16, float>(
                static_cast<int16>(abs(actual_val - expected_val)));
          }
        }
      }

      float avg_error = sum_of_errors / kNumOfDestFrames;
      EXPECT_LE(avg_error, kMaxAvgError);
    }
  }
}

}  // namespace audio
}  // namespace cobalt
