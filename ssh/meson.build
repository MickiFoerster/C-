project(
  'ssh_tools', 
  'c',
)

executable(
  'ssh2_exec',
  sources: ['ssh2_exec.c'],
  dependencies: [meson.get_compiler('c').find_library('ssh2')],
  c_args: ['-std=c11', '-Wall', '-Werror']
)
