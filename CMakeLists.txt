cmake_minimum_required(VERSION 2.8)
project (DEBUGCANVAS)

add_executable(debugcanvas
  src/main.cc

  # bgfx example setup stuff, to be nuked.
  third_party/bgfx/examples/common/entry/camera.cpp
  third_party/bgfx/examples/common/entry/cmd.cpp
  third_party/bgfx/examples/common/entry/dbg.cpp
  third_party/bgfx/examples/common/entry/entry.cpp
  third_party/bgfx/examples/common/entry/entry_android.cpp
  third_party/bgfx/examples/common/entry/entry_asmjs.cpp
  third_party/bgfx/examples/common/entry/entry_linux.cpp
  third_party/bgfx/examples/common/entry/entry_nacl.cpp
  third_party/bgfx/examples/common/entry/entry_qnx.cpp
  third_party/bgfx/examples/common/entry/entry_sdl.cpp
  third_party/bgfx/examples/common/entry/entry_windows.cpp
  third_party/bgfx/examples/common/entry/input.cpp
  third_party/bgfx/examples/common/font/font_manager.cpp
  third_party/bgfx/examples/common/font/text_buffer_manager.cpp
  third_party/bgfx/examples/common/font/text_metrics.cpp
  third_party/bgfx/examples/common/font/utf8.cpp
  third_party/bgfx/examples/common/cube_atlas.cpp

  # bgfx
  third_party/bgfx/src/bgfx.cpp
  third_party/bgfx/src/glcontext_egl.cpp
  third_party/bgfx/src/glcontext_glx.cpp
  third_party/bgfx/src/glcontext_ppapi.cpp
  third_party/bgfx/src/glcontext_wgl.cpp
  third_party/bgfx/src/image.cpp
  third_party/bgfx/src/renderer_d3d11.cpp
  third_party/bgfx/src/renderer_d3d9.cpp
  third_party/bgfx/src/renderer_gl.cpp
  third_party/bgfx/src/renderer_null.cpp
  third_party/bgfx/src/vertexdecl.cpp

  )

set (CMAKE_CXX_FLAGS "/W4 /WX /wd4530 /wd4244 /wd4127 /wd4512 /wd4245 /wd4201 /wd4324 /wd4611")

add_definitions (
  -D_CRT_SECURE_NO_WARNINGS
  -DBX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1
  -D__STDC_LIMIT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_CONSTANT_MACROS
  )

include_directories(
  third_party
  third_party/bx/include/compat/msvc
  third_party/bx/include
  third_party/bgfx/include

  # XXX
  third_party/bgfx/examples/common
  )
