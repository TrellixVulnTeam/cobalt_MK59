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

#include "cobalt/media_capture/media_device_info.h"

#include <string>

namespace cobalt {
namespace media_capture {

MediaDeviceInfo::MediaDeviceInfo(
    script::ScriptValueFactory* script_value_factory,
    MediaDeviceKind kind,
    const std::string& label)
        : script_value_factory_(script_value_factory),
          kind_(kind),
          label_(label) {
}

}  // namespace media_capture
}  // namespace cobalt
