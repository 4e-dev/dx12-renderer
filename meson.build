project(
  'App',
  'cpp',
  default_options : [
    'cpp_std=c++latest',
    'warning_level=1',
    'buildtype=debug'
  ]
)

cpp = meson.get_compiler('cpp')

source_files = files(
  'Engine/App.cpp'
)

includes = include_directories(
  'Engine'
)

d3d12_lib = cpp.find_library('d3d12')
dxgi_lib = cpp.find_library('dxgi')
dxguid_lib = cpp.find_library('dxguid')

executable(
  'App',
  source_files,
  include_directories : includes,
  dependencies : [
    d3d12_lib,
    dxgi_lib,
    dxguid_lib
  ],
  win_subsystem : 'windows'
)
