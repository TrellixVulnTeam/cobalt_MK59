#!/usr/bin/python
# Copyright 2018 The Cobalt Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Builds a symlink farm pointing to specified subdirectories of the input dir.
"""

import argparse
import logging
import os
import shutil
import stat
import sys
import traceback

import _env # pylint: disable=unused-import

# The name of an environment variable that when set to |'1'|, signals to us that
# we should log all output directories that we have populated.
_SHOULD_LOG_ENV_KEY = 'STARBOARD_GYP_SHOULD_LOG_COPIES'
_USE_WINDOWS_SYMLINK = sys.platform in ['win32', 'cygwin']

if _USE_WINDOWS_SYMLINK:
  import starboard.build.win_symlink as win_symlink


def EscapePath(path):
  """Returns a path with spaces escaped."""
  return path.replace(' ', '\\ ')

def _ClearDir(path):
  path = os.path.normpath(path)
  if not os.path.exists(path): # Works for symlinks for both *nix and Windows.
    return
  if _USE_WINDOWS_SYMLINK:
    win_symlink.RmtreeShallow(path)
  else:
    # Note that shutil.rmtree() has undocumented behavior on *nix systems
    # for subitems which are symlink directories. The symlink is deleted
    # but the subitems in the referenced directory are NOT deleted.
    shutil.rmtree(path)


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-i', dest='input_dir', required=True, help='input directory')
  parser.add_argument(
      '-o', dest='output_dir', required=True, help='output directory')
  parser.add_argument(
      '-s', dest='stamp_file', required=True,
      help='stamp file to update after the output directory is populated')
  parser.add_argument(
      'subdirs', metavar='subdirs', nargs='*',
      help='subdirectories within both the input and output directories')
  options = parser.parse_args(argv[1:])

  if os.environ.get(_SHOULD_LOG_ENV_KEY, None) == '1':
    log_level = logging.INFO
  else:
    log_level = logging.WARNING
  logging.basicConfig(level=log_level, format='COLLECT CONTENT: %(message)s')

  logging.info('< %s', options.input_dir)
  logging.info('> %s', options.output_dir)
  for subdir in options.subdirs:
    logging.info('+ %s', subdir)

  if os.path.isdir(options.output_dir):
    _ClearDir(options.output_dir)

  for subdir in options.subdirs:
    src_path = os.path.abspath(
        EscapePath(os.path.join(options.input_dir, subdir)))
    dst_path = os.path.abspath(
        EscapePath(os.path.join(options.output_dir, subdir)))

    dst_dir = os.path.dirname(dst_path)
    rel_path = os.path.relpath(src_path, dst_dir)

    logging.info('%s => %s', dst_path, rel_path)

    if not os.path.exists(dst_dir):
      try:
        os.makedirs(dst_dir)
      except Exception as err:
        msg = 'Error: ' + str(err)
        if os.path.isdir(dst_dir):
          msg += ' path is a directory'
        elif os.path.isfile(dst_dir):
          msg += ' path is a file'
        else:
          msg += ' path points to an unknown type'
        logging.error(msg)

    if _USE_WINDOWS_SYMLINK:
      win_symlink.CreateReparsePoint(src_path, dst_path)
    else:
      os.symlink(rel_path, dst_path)

  if options.stamp_file:
    with open(options.stamp_file, 'w') as stamp_file:
      stamp_file.write('\n'.join(options.subdirs))


if __name__ == '__main__':
  sys.exit(main(sys.argv))
