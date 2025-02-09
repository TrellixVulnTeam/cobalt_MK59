// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/main_hook.h"
#include "base/test/test_suite.h"
#include "crypto/nss_util.h"
#include "starboard/client_porting/wrap_main/wrap_main.h"

int TestSuiteRun(int argc, char** argv) {
  MainHook hook(NULL, argc, argv);
#if defined(USE_NSS)
  // This is most likely not needed, but it basically replaces a similar call
  // that was performed on test_support_base.
  // TODO(rvargas) Bug 79359: remove this.
  crypto::EnsureNSSInit();
#endif  // defined(USE_NSS)
  return base::TestSuite(argc, argv).Run();
}

STARBOARD_WRAP_SIMPLE_MAIN(TestSuiteRun);
