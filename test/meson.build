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
    'packets.cpp',
    'messages.cpp',
    'hexdump.cpp',
    'public' / 'channelid.cpp',
    'public' / 'peerid.cpp',
    'public' / 'cookie.cpp',
    'public' / 'message.cpp',
    'public' / 'packet.cpp',
  ]

  private_test_src = [
    'runner.cpp',
    'packets.cpp',
    'messages.cpp',
    'hexdump.cpp',
    'private' / 'memory' / 'packet_block.cpp',
    'private' / 'memory' / 'packet_pool.cpp',
    'private' / 'memory' / 'packet_buffer.cpp',
    'private' / 'support' / 'timeouts.cpp',
    'private' / 'support' / 'exponential_backoff.cpp',
    'private' / 'pipe' / 'ingress' / 'de_envelope.cpp',
    'private' / 'pipe' / 'ingress' / 'route.cpp',
    'private' / 'pipe' / 'ingress' / 'validate.cpp',
    'private' / 'pipe' / 'ingress' / 'channel_assign.cpp',
    'private' / 'pipe' / 'ingress' / 'message_parsing.cpp',
    'private' / 'pipe' / 'ingress' / 'state_handling.cpp',
    'private' / 'pipe' / 'ingress.cpp',
    'private' / 'pipe' / 'egress' / 'enqueue_message.cpp',
    'private' / 'pipe' / 'egress' / 'message_bundling.cpp',
    'private' / 'pipe' / 'egress' / 'add_checksum.cpp',
    'private' / 'pipe' / 'egress' / 'out_buffer.cpp',
    'private' / 'pipe' / 'egress' / 'callback.cpp',
    'private' / 'pipe' / 'egress.cpp',
    'private' / 'fsm' / 'channel_responder.cpp',
    'private' / 'fsm' / 'channel_initiator.cpp',
    'private' / 'fsm' / 'data.cpp',
    'private' / 'fsm' / 'registry.cpp',
    'private' / 'fsm' / 'default.cpp',
    'private' / 'internal' / 'api.cpp',
    'private' / 'channels.cpp',
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
