project(
  'app',
  'cpp',
  default_options : [
    'cpp_std=c++23',
    'warning_level=1',
    'buildtype=debug',
    'backend=vs'
  ]
)

source_files = files('src/app.cpp')
includes = include_directories('src')

compiler = meson.get_compiler('cpp')
dependency_names = ['d3d12', 'dxgi', 'dxguid']
dependencies = []
foreach dependency : dependency_names
  dependencies += compiler.find_library(dependency)
endforeach

executable(
  'app',
  source_files,
  include_directories : includes,
  dependencies : dependencies,
  win_subsystem : 'windows'
)
