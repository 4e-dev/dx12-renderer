project(
  'app',
  'cpp',
  default_options : [
    'cpp_std=c++20',
    'warning_level=1',
    'buildtype=debug',
    'backend=vs'
  ]
)

source_files = files('src/app.cpp')
includes = include_directories('src')

compiler = meson.get_compiler('cpp')
depnames = ['d3d12', 'dxgi', 'dxguid']
deps = []
foreach dep : depnames
  deps += compiler.find_library(dep)
endforeach

executable(
  'app',
  source_files,
  include_directories : includes,
  dependencies : deps,
  win_subsystem : 'windows'
)
