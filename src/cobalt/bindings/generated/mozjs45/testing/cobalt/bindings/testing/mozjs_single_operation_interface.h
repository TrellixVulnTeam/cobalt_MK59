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

// clang-format off

// This file has been auto-generated by bindings/code_generator_cobalt.py. DO NOT MODIFY!
// Auto-generated from template: bindings/mozjs45/templates/callback-interface.h.template

#ifndef MozjsSingleOperationInterface_h
#define MozjsSingleOperationInterface_h

#include "cobalt/script/callback_interface_traits.h"
// Headers for other bindings wrapper classes
#include "cobalt/bindings/testing/single_operation_interface.h"

#include "cobalt/script/mozjs-45/weak_heap_object.h"
#include "third_party/mozjs-45/js/src/jsapi.h"

namespace cobalt {
namespace bindings {
namespace testing {

class MozjsSingleOperationInterface : public SingleOperationInterface {
 public:
  typedef SingleOperationInterface BaseType;

  MozjsSingleOperationInterface(
      JSContext* context, JS::HandleValue implementing_object_value);
  base::optional<int32_t > HandleCallback(
      const scoped_refptr<script::Wrappable>& callback_this,
      const scoped_refptr<ArbitraryInterface>& value,
      bool* had_exception) const override;

  JSObject* handle() const { return implementing_object_.GetObject(); }
  const JS::Value& value() const { return implementing_object_.GetValue(); }
  bool WasCollected() const { return implementing_object_.WasCollected(); }
  void Trace(JSTracer* js_tracer) { implementing_object_.Trace(js_tracer); }

 private:
  JSContext* context_;
  script::mozjs::WeakHeapObject implementing_object_;
};

}  // namespace testing
}  // namespace bindings
}  // namespace cobalt
namespace cobalt {
namespace script {
// Explicit instantiation of CallbackInterfaceTraits struct so we can infer
// the type of the generated class from the type of the callback interface.
template<>
struct CallbackInterfaceTraits<cobalt::bindings::testing::SingleOperationInterface> {
  typedef cobalt::bindings::testing::MozjsSingleOperationInterface MozjsCallbackInterfaceClass;
};
}  // namespace script
}  // namespace cobalt

#endif  // MozjsSingleOperationInterface_h
