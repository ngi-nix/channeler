##############################################################################
# Project
project('channeler', 'cpp',
  version: '0.1.0',
  license: 'GPLv3',
  meson_version: '>=0.53.0',
  default_options: [
    'cpp_std=c++17',
    'default_library=both',
  ])


##############################################################################
# Versioning, project and libraries

# There are so many versioning schemes, each of which is incompatible with
# others. We'll manage best by keeping things simple:
#
# - The package version follows semver
# - The library version is the package version
# - The ABI version, i.e. the compatibility we expect, is the major
#   component of the package
_splitver = meson.project_version().split('.')
PACKAGE_MAJOR = _splitver[0]
PACKAGE_MINOR = _splitver[1]
PACKAGE_PATCH = _splitver[2]

ABI_VERSION = PACKAGE_MAJOR
LIB_VERSION = meson.project_version()
PACKAGE_VERSION = meson.project_version()

##############################################################################
# Configuration

conf_data = configuration_data()
compiler = meson.get_compiler('cpp')

host_type = ''
if host_machine.system() in [ 'cygwin', 'darwin', 'dragonfly', 'freebsd', 'gnu', 'linux', 'netbsd' ]
  host_type = 'posix'
elif host_machine.system() == 'windows'
  host_type = 'win32'
elif host_machine.system().startswith('android')
  host_type = 'android'
endif
summary('Host platform', host_type, section: 'Platform')

# For Windows, try to determine the SDK version.
winsdk_ver = -1
if compiler.has_header('ntverp.h')
  code = '''#include <iostream>
#include <ntverp.h>

int main()
{
  std::cout << VER_PRODUCTMAJORVERSION;
}
'''
  result = compiler.run(code, name: 'Windows SDK version check.')
  winsdk_ver = result.stdout().to_int()
  summary('Windows SDK major version', winsdk_ver, section: 'Platform')
endif


### Compiler flags
compiler_id = compiler.get_id()

cpp_lib_is_building = []
cpp_args = []

posix_common_args = [
  '-Wall', '-Wextra', '-Wpedantic', '-Wshadow', '-Wstrict-aliasing',
  '-Wstrict-overflow=5', '-Wcast-align',
  '-Wpointer-arith', '-Wcast-qual', '-Wold-style-cast',
]
if compiler_id == 'clang'
  cpp_args += posix_common_args + [
    '-Wabi',
  ]
  cpp_lib_is_building = ['-DCHANNELER_IS_BUILDING=1']
elif compiler_id == 'gcc'
  cpp_args += posix_common_args + [
    '-Wstrict-null-sentinel',
  ]
  cpp_lib_is_building = ['-DCHANNELER_IS_BUILDING=1']
elif compiler_id == 'msvc'
  cpp_args += [
    '/wd4250', '/wd4251', '/wd4275',
  ]
  cpp_lib_is_building = ['/DCHANNELER_IS_BUILDING=1']
endif

bt = get_option('buildtype')
if bt in ['debug']
  if compiler_id == 'clang'
    cpp_args += ['-ggdb', '-DDEBUG']
  elif compiler_id == 'gcc'
    cpp_args += ['-g3', '-DDEBUG']
  elif compiler_id == 'msvc'
    cpp_args += ['/FS', '/DDEBUG=1']
  endif
elif bt in ['debugoptimized']
  if compiler_id == 'clang'
    cpp_args += ['-ggdb', '-DNDEBUG']
  elif compiler_id == 'gcc'
    cpp_args += ['-g3', '-DNDEBUG']
  elif compiler_id == 'msvc'
    cpp_args += ['/FS', '/DNDEBUG=1']
  endif
else
  posix_release_args = [
    '-fvisibility=hidden', '-fvisibility-inlines-hidden',
    '-DNDEBUG',
  ]
  if compiler_id == 'clang'
    cpp_args += posix_release_args
  elif compiler_id == 'gcc'
    cpp_args += posix_release_args
  elif compiler_id == 'msvc'
    cpp_args += ['/Oi', '/DNDEBUG=1']
  endif
endif

link_args = []
if host_type == 'android'
  # Only posix compilers supported (?)
  cpp_args += [
    '-DANDROID_STL=c++_shared',
    '-fexceptions', '-frtti',
  ]
  link_args = [
    '-lstdc++', '-lm',
  ]
endif


# Make things work with MSVC and Windows SDK <10
if compiler_id == 'msvc' and winsdk_ver < 10
  cpp_args += ['/permissive']
endif

add_project_arguments(cpp_args, language: 'cpp')


### Version and package information
conf_data.set_quoted('CHANNELER_PACKAGE_MAJOR', PACKAGE_MAJOR)
conf_data.set_quoted('CHANNELER_PACKAGE_MINOR', PACKAGE_MINOR)
conf_data.set_quoted('CHANNELER_PACKAGE_PATCH', PACKAGE_PATCH)
conf_data.set_quoted('CHANNELER_PACKAGE_VERSION', PACKAGE_VERSION)
conf_data.set_quoted('CHANNELER_ABI_VERSION', ABI_VERSION)
conf_data.set_quoted('CHANNELER_LIB_VERSION', LIB_VERSION)

conf_data.set_quoted('CHANNELER_PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('CHANNELER_PACKAGE_URL', 'https://gitlab.com/interpeer/channeler')

### Host platform details
conf_data.set('CHANNELER_BIGENDIAN', host_machine.endian() == 'big')

### Headers

# TODO?

### Types

compiler.sizeof('int32_t', prefix: '#include <stdint.h>')
compiler.sizeof('uint32_t', prefix: '#include <stdint.h>')
compiler.sizeof('int64_t', prefix: '#include <stdint.h>')
compiler.sizeof('uint64_t', prefix: '#include <stdint.h>')
compiler.sizeof('size_t', prefix: '#include <stdint.h>')
compiler.sizeof('ssize_t', prefix: '#include <stdint.h>')


### Set values from options

configure_file(
  input: 'build-config.h.in',
  output: 'build-config.h',
  configuration: conf_data
)



##############################################################################
# Meson Wrap Dependencies
liberate = subproject('liberate')
# TODO not yet used packeteer = subproject('packeteer')
gtest = subproject('gtest')
# FIXME? clipp = subproject('muellan-clipp')
# FIXME thread = dependency('threads', required: true)

##############################################################################
# Library

includes = include_directories(
  'include',
)

libincludes = include_directories(
  'lib',
)


install_headers(
  'include' / 'channeler.h',
)

install_headers(
  'include' / 'channeler' / 'channelid.h',
  'include' / 'channeler' / 'error.h',
  'include' / 'channeler' / 'packet.h',
  'include' / 'channeler' / 'peerid.h',
  'include' / 'channeler' / 'protoid.h',
  'include' / 'channeler' / 'version.h',
  'include' / 'channeler' / 'visibility.h',

  subdir: 'channeler',
)

libsrc = [
  'lib' / 'channelid.cpp',
  'lib' / 'error.cpp',
  'lib' / 'packet.cpp',
  'lib' / 'peerid.cpp',
  'lib' / 'version.cpp',
]


lib = library('channeler', libsrc,
    include_directories: [includes, libincludes],
    dependencies: [
      liberate.get_variable('liberate_dep'),
    ],
    link_args: link_args,
    cpp_args: cpp_lib_is_building,
    version: LIB_VERSION,
    soversion: ABI_VERSION,
    install: true)

channeler_dep = declare_dependency(
    include_directories: [includes],
    dependencies: [
      liberate.get_variable('liberate_dep'),
    ],
    link_with: [lib],
    link_args: link_args,
    version: LIB_VERSION,
)



##############################################################################
# Linter, etc.
cppcheck = find_program('cppcheck', required: false)

if cppcheck.found()
  run_target('cppcheck', command: [
    cppcheck.path(),
    '--cppcheck-build-dir=@0@/cppcheck'.format(meson.current_build_dir()),
    '--enable=all', '-v',
    '--suppress=missingIncludeSystem',
    '--inline-suppr',
    '-I', '@0@/include'.format(meson.current_source_dir()),
    '-I', '@0@/lib'.format(meson.current_source_dir()),
    '-I', meson.current_build_dir(),
    '--std=c++17',
    cpp_lib_is_building,
    '--quiet',
    '@0@/lib'.format(meson.current_source_dir()),
  ])
endif

oclint = find_program('oclint', required: false)

if oclint.found()
  oclint_config = custom_target('oclint_config',
      output: '.oclint',
      input: '.oclint',
      command: ['cp', '@INPUT@', '@OUTPUT@'],
      install: false,
  )

  run_target('oclint', command: [
      oclint.path(),
      libsrc,
      '-o', '@0@/oclint.log'.format(meson.current_build_dir()),
      '--',
      '-I', '@0@/include'.format(meson.current_source_dir()),
      '-I', '@0@/lib'.format(meson.current_source_dir()),
      '-I', meson.current_build_dir(),
      '--std=@0@'.format(get_option('cpp_std')),
      cpp_args,
      cpp_lib_is_building,
      '-DOCLINT_IS_RUNNING',
    ],
    depends: oclint_config,
  )
endif


##############################################################################
# Subdirectories
subdir('test')
