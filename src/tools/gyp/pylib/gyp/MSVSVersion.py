# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handle version information related to Visual Stuio."""

import errno
import os
import re
import subprocess
import sys
import gyp


class VisualStudioVersion(object):
  """Information regarding a version of Visual Studio."""

  def __init__(self, short_name, description,
               solution_version, project_version, flat_sln, uses_vcxproj,
               path, sdk_based, default_toolset=None):
    self.short_name = short_name
    self.description = description
    self.solution_version = solution_version
    self.project_version = project_version
    self.flat_sln = flat_sln
    self.uses_vcxproj = uses_vcxproj
    self.path = path
    self.sdk_based = sdk_based
    self.default_toolset = default_toolset

  def ShortName(self):
    return self.short_name

  def Description(self):
    """Get the full description of the version."""
    return self.description

  def SolutionVersion(self):
    """Get the version number of the sln files."""
    return self.solution_version

  def ProjectVersion(self):
    """Get the version number of the vcproj or vcxproj files."""
    return self.project_version

  def FlatSolution(self):
    return self.flat_sln

  def UsesVcxproj(self):
    """Returns true if this version uses a vcxproj file."""
    return self.uses_vcxproj

  def ProjectExtension(self):
    """Returns the file extension for the project."""
    return self.uses_vcxproj and '.vcxproj' or '.vcproj'

  def Path(self):
    """Returns the path to Visual Studio installation."""
    return self.path

  def ToolPath(self, tool):
    """Returns the path to a given compiler tool. """
    return os.path.normpath(os.path.join(self.path, "VC/bin", tool))

  def DefaultToolset(self):
    """Returns the msbuild toolset version that will be used in the absence
    of a user override."""
    return self.default_toolset

  def SetupScript(self, target_arch):
    """Returns a command (with arguments) to be used to set up the
    environment."""
    # Check if we are running in the SDK command line environment and use
    # the setup script from the SDK if so. |target_arch| should be either
    # 'x86' or 'x64'.
    assert target_arch in ('x86', 'x64')
    sdk_dir = os.environ.get('WindowsSDKDir')
    if self.sdk_based and sdk_dir:
      return [os.path.normpath(os.path.join(sdk_dir, 'Bin/SetEnv.Cmd')),
              '/' + target_arch]
    else:
      # We don't use VC/vcvarsall.bat for x86 because vcvarsall calls
      # vcvars32, which it can only find if VS??COMNTOOLS is set, which it
      # isn't always.
      if target_arch == 'x86':
        return [os.path.normpath(
          os.path.join(self.path, 'Common7/Tools/vsvars32.bat'))]
      else:
        assert target_arch == 'x64'
        arg = 'x86_amd64'
        if (os.environ.get('PROCESSOR_ARCHITECTURE') == 'AMD64' or
            os.environ.get('PROCESSOR_ARCHITEW6432') == 'AMD64'):
          # Use the 64-on-64 compiler if we can.
          arg = 'amd64'
        return [os.path.normpath(
            os.path.join(self.path, 'VC/vcvarsall.bat')), arg]


def _RegistryQueryBase(sysdir, key, value):
  """Use reg.exe to read a particular key.

  While ideally we might use the win32 module, we would like gyp to be
  python neutral, so for instance cygwin python lacks this module.

  Arguments:
    sysdir: The system subdirectory to attempt to launch reg.exe from.
    key: The registry key to read from.
    value: The particular value to read.
  Return:
    stdout from reg.exe, or None for failure.
  """
  # Skip if not on Windows or Python Win32 setup issue
  if sys.platform not in ('win32', 'cygwin'):
    return None
  # Setup params to pass to and attempt to launch reg.exe
  cmd = [os.path.join(os.environ.get('WINDIR', ''), sysdir, 'reg.exe'),
         'query', key]
  if value:
    cmd.extend(['/v', value])
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  # Obtain the stdout from reg.exe, reading to the end so p.returncode is valid
  # Note that the error text may be in [1] in some cases
  text = p.communicate()[0]
  # Check return code from reg.exe; officially 0==success and 1==error
  if p.returncode:
    return None
  return text


def _RegistryQuery(key, value=None):
  """Use reg.exe to read a particular key through _RegistryQueryBase.

  First tries to launch from %WinDir%\Sysnative to avoid WoW64 redirection. If
  that fails, it falls back to System32.  Sysnative is available on Vista and
  up and available on Windows Server 2003 and XP through KB patch 942589. Note
  that Sysnative will always fail if using 64-bit python due to it being a
  virtual directory and System32 will work correctly in the first place.

  KB 942589 - http://support.microsoft.com/kb/942589/en-us.

  Arguments:
    key: The registry key.
    value: The particular registry value to read (optional).
  Return:
    stdout from reg.exe, or None for failure.
  """
  text = None
  try:
    text = _RegistryQueryBase('Sysnative', key, value)
  except OSError, e:
    if e.errno == errno.ENOENT:
      text = _RegistryQueryBase('System32', key, value)
    else:
      raise
  return text


def _RegistryGetValue(key, value):
  """Use reg.exe to obtain the value of a registry key.

  Args:
    key: The registry key.
    value: The particular registry value to read.
  Return:
    contents of the registry key's value, or None on failure.
  """
  text = _RegistryQuery(key, value)
  if not text:
    return None
  # Extract value.
  match = re.search(r'REG_\w+\s+([^\r]+)\r\n', text)
  if not match:
    return None
  return match.group(1)


def _RegistryKeyExists(key):
  """Use reg.exe to see if a key exists.

  Args:
    key: The registry key to check.
  Return:
    True if the key exists
  """
  if not _RegistryQuery(key):
    return False
  return True


def _CreateVersion(name, path, sdk_based=False):
  """Sets up MSVS project generation.

  Setup is based off the GYP_MSVS_VERSION environment variable or whatever is
  autodetected if GYP_MSVS_VERSION is not explicitly specified. If a version is
  passed in that doesn't match a value in versions python will throw a error.
  """
  if path:
    path = os.path.normpath(path)
  versions = {
      '2017': VisualStudioVersion('2017',
                                  'Visual Studio 2017',
                                  solution_version='12.0',
                                  project_version='14.0',
                                  flat_sln=False,
                                  uses_vcxproj=True,
                                  path=path,
                                  sdk_based=sdk_based,
                                  default_toolset='v141'),
      '2012': VisualStudioVersion('2012',
                                  'Visual Studio 2012',
                                  solution_version='12.00',
                                  project_version='4.0',
                                  flat_sln=False,
                                  uses_vcxproj=True,
                                  path=path,
                                  sdk_based=sdk_based,
                                  default_toolset='v110'),
      '2012e': VisualStudioVersion('2012e',
                                   'Visual Studio 2012',
                                   solution_version='12.00',
                                   project_version='4.0',
                                   flat_sln=True,
                                   uses_vcxproj=True,
                                   path=path,
                                   sdk_based=sdk_based,
                                   default_toolset='v110'),
      '2010': VisualStudioVersion('2010',
                                  'Visual Studio 2010',
                                  solution_version='11.00',
                                  project_version='4.0',
                                  flat_sln=False,
                                  uses_vcxproj=True,
                                  path=path,
                                  sdk_based=sdk_based),
      '2010e': VisualStudioVersion('2010e',
                                   'Visual Studio 2010',
                                   solution_version='11.00',
                                   project_version='4.0',
                                   flat_sln=True,
                                   uses_vcxproj=True,
                                   path=path,
                                   sdk_based=sdk_based),
      '2008': VisualStudioVersion('2008',
                                  'Visual Studio 2008',
                                  solution_version='10.00',
                                  project_version='9.00',
                                  flat_sln=False,
                                  uses_vcxproj=False,
                                  path=path,
                                  sdk_based=sdk_based),
      '2008e': VisualStudioVersion('2008e',
                                   'Visual Studio 2008',
                                   solution_version='10.00',
                                   project_version='9.00',
                                   flat_sln=True,
                                   uses_vcxproj=False,
                                   path=path,
                                   sdk_based=sdk_based),
      '2005': VisualStudioVersion('2005',
                                  'Visual Studio 2005',
                                  solution_version='9.00',
                                  project_version='8.00',
                                  flat_sln=False,
                                  uses_vcxproj=False,
                                  path=path,
                                  sdk_based=sdk_based),
      '2005e': VisualStudioVersion('2005e',
                                   'Visual Studio 2005',
                                   solution_version='9.00',
                                   project_version='8.00',
                                   flat_sln=True,
                                   uses_vcxproj=False,
                                   path=path,
                                   sdk_based=sdk_based),
  }
  return versions[str(name)]


def _ConvertToCygpath(path):
  """Convert to cygwin path if we are using cygwin."""
  if sys.platform == 'cygwin':
    p = subprocess.Popen(['cygpath', path], stdout=subprocess.PIPE)
    path = p.communicate()[0].strip()
  return path


def _DetectVisualStudioVersions(versions_to_check, force_express,
                                vs_install_dir=None):
  """Collect the list of installed visual studio versions.

  Returns:
    A list of visual studio versions installed in descending order of
    usage preference.
    Base this on the registry and a quick check if devenv.exe exists.
    Only versions 8-10 are considered.
    Possibilities are:
      2005(e) - Visual Studio 2005 (8)
      2008(e) - Visual Studio 2008 (9)
      2010(e) - Visual Studio 2010 (10)
      2012(e) - Visual Studio 2012 (11)
    Where (e) is e for express editions of MSVS and blank otherwise.
  """
  version_to_year = {
      '8.0': '2005', '9.0': '2008', '10.0': '2010', '11.0': '2012',
      '14.0': '2015', '15.0': '2017'}
  versions = []
  for version in versions_to_check:
    if version == '15.0':
      # Version 15 does not include registry entries.
      if vs_install_dir:
        path = os.path.join(vs_install_dir, r'Common7\IDE')
      else:
        path = r'C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE'
      path = _ConvertToCygpath(path)
      full_path = os.path.join(path, 'devenv.exe')
      if os.path.exists(full_path) and version in version_to_year:
        versions.append(_CreateVersion(version_to_year[version],
            os.path.join(path, '..', '..')))
      continue
    # Old method of searching for which VS version is installed
    # We don't use the 2010-encouraged-way because we also want to get the
    # path to the binaries, which it doesn't offer.
    keys = [r'HKLM\Software\Microsoft\VisualStudio\%s' % version,
            r'HKLM\Software\Wow6432Node\Microsoft\VisualStudio\%s' % version,
            r'HKLM\Software\Microsoft\VCExpress\%s' % version,
            r'HKLM\Software\Wow6432Node\Microsoft\VCExpress\%s' % version]
    for index in range(len(keys)):
      path = _RegistryGetValue(keys[index], 'InstallDir')
      if not path:
        continue
      path = _ConvertToCygpath(path)
      # Check for full.
      full_path = os.path.join(path, 'devenv.exe')
      express_path = os.path.join(path, 'vcexpress.exe')
      if (not force_express and os.path.exists(full_path) and
          version in version_to_year):
        # Add this one.
        versions.append(_CreateVersion(version_to_year[version],
            os.path.join(path, '..', '..')))
      # Check for express.
      elif os.path.exists(express_path) and version in version_to_year:
        # Add this one.
        versions.append(_CreateVersion(version_to_year[version] + 'e',
            os.path.join(path, '..', '..')))

    # The old method above does not work when only SDK is installed.
    keys = [r'HKLM\Software\Microsoft\VisualStudio\SxS\VC7',
            r'HKLM\Software\Wow6432Node\Microsoft\VisualStudio\SxS\VC7']
    for index in range(len(keys)):
      path = _RegistryGetValue(keys[index], version)
      if not path:
        continue
      path = _ConvertToCygpath(path)
      if version in version_to_year:
        versions.append(_CreateVersion(version_to_year[version] + 'e',
            os.path.join(path, '..'), sdk_based=True))
  return versions


def SelectVisualStudioVersion(version='auto'):
  """Select which version of Visual Studio projects to generate.

  Arguments:
    version: Hook to allow caller to force a particular version (vs auto).
  Returns:
    An object representing a visual studio project format version.
  """
  # In auto mode, check environment variable for override.
  if version == 'auto':
    version = os.environ.get('GYP_MSVS_VERSION', 'auto')
  version_map = {
    'auto': ('10.0', '9.0', '8.0', '11.0', '12.0'),
    '2005': ('8.0',),
    '2005e': ('8.0',),
    '2008': ('9.0',),
    '2008e': ('9.0',),
    '2010': ('10.0',),
    '2010e': ('10.0',),
    '2012': ('11.0',),
    '2012e': ('11.0',),
    '2017': ('15.0',),
  }

  version = str(version)
  vs_install_dir = os.environ.get('VS_INSTALL_DIR')
  versions = _DetectVisualStudioVersions(version_map[version], 'e' in version,
                                         vs_install_dir)
  if not versions:
    if version == 'auto':
      # Default to 2005 if we couldn't find anything
      return _CreateVersion('2005', None)
    else:
      return _CreateVersion(version, None)
  return versions[0]
