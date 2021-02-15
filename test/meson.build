##############################################################################
# Tests

# Tests in a subproject are complicicated. You need all the compile and
# link flags from the enclosing project, etc.
if not meson.is_subproject()

  # See https://github.com/google/googletest/issues/813
  test_args = []
  if host_machine.system() == 'cygwin'
    test_args += ['-D_POSIX_C_SOURCE=200809L']
  endif

  # Google test issues this warning; disable it in *test* code only.
  if compiler_id == 'msvc'
    test_args = [
      '/wd4389',
    ]
  endif

  # We're building two tests:
  # - public_tests include *only* public headers
  # - private_tests include private headers

  public_test_src = [
    'runner.cpp',
    'public' / 'peerid.cpp',
    'public' / 'packet.cpp',
  ]

  private_test_src = [
    'runner.cpp',
  ]

  public_tests = executable('public_tests', public_test_src,
      dependencies: [
        channeler_dep,
        gtest.get_variable('gtest_dep'),
      ],
      cpp_args: test_args,
  )
  test('public_tests', public_tests)

  # Due to symbol visibility, private tests won't link for non-debug builds
  if bt in ['debug', 'debugoptimized']
    private_tests = executable('private_tests', private_test_src,
        include_directories: [libincludes],  # Also private headers
        dependencies: [
          channeler_dep,
          gtest.get_variable('gtest_dep'),
        ],
        cpp_args: test_args,
    )
    test('private_tests', private_tests)
  endif

endif
