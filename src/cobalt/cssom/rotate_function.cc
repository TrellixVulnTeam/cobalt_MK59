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

#include "cobalt/cssom/rotate_function.h"

#include "cobalt/cssom/transform_function_visitor.h"

namespace cobalt {
namespace cssom {

void RotateFunction::Accept(TransformFunctionVisitor* visitor) const {
  visitor->VisitRotate(this);
}

}  // namespace cssom
}  // namespace cobalt
