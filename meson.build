project('dx12-game', 'cpp',
  version: '1.0',
  license: [ 'MIT' ],
  default_options: [
    'warning_level=1',
    'buildtype=debug'])

compiler = meson.get_compiler('cpp')

source_files = files(
  'src/app.cpp',
  'src/debug/debug.cpp')

includes = include_directories(
  'src/',
  'src/core/',
  'src/debug/')

dependencies = []
foreach dependency : ['d3d12', 'dxgi', 'dxguid']
  dependencies += compiler.find_library(dependency, required : true)
endforeach

executable(
  'app',
  source_files,
  include_directories : includes,
  dependencies : dependencies,
  win_subsystem : 'windows')
