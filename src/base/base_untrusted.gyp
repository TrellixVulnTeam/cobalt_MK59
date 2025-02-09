# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common_untrusted.gypi',
    'base.gypi',
  ],
  'conditions': [
    ['disable_nacl==0', {
      'targets': [
        {
          'target_name': 'base_untrusted',
          'type': 'none',
          'variables': {
            'base_target': 1,
            'nacl_untrusted_build': 1,
            'nlib_target': 'libbase_untrusted.a',
            'build_glibc': 1,
            'build_newlib': 1,
            'sources': [
              'string16.cc',
              'sync_socket_nacl.cc',
              'third_party/nspr/prtime.cc',
              'time_posix.cc',
            ],
          },
          'dependencies': [
            '<(DEPTH)/native_client/tools.gyp:prep_toolchain',
          ],
        },
      ],
    }],
  ],
}
