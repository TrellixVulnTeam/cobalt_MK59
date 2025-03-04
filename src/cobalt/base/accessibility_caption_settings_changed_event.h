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

#ifndef COBALT_BASE_ACCESSIBILITY_CAPTION_SETTINGS_CHANGED_EVENT_H_
#define COBALT_BASE_ACCESSIBILITY_CAPTION_SETTINGS_CHANGED_EVENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/string_util.h"
#include "cobalt/base/event.h"

namespace base {

class AccessibilityCaptionSettingsChangedEvent : public Event {
 public:
  AccessibilityCaptionSettingsChangedEvent() {}

  BASE_EVENT_SUBCLASS(AccessibilityCaptionSettingsChangedEvent);
};

}  // namespace base

#endif  // COBALT_BASE_ACCESSIBILITY_CAPTION_SETTINGS_CHANGED_EVENT_H_
