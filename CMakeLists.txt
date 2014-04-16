cmake_minimum_required(VERSION 2.8)
project (DEBUGCANVAS)

# XXX Windows only.
set (CMAKE_CXX_FLAGS "/W4 /WX /wd4530 /wd4244 /wd4127 /wd4512 /wd4245 /wd4201 /wd4324 /wd4611")

add_definitions (
  -DBX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1
  -D_CRT_SECURE_NO_WARNINGS
  -D__STDC_LIMIT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_CONSTANT_MACROS

  # XXX Debug only.
  -DBGFX_CONFIG_DEBUG=1

  # XXX Windows only.
  -DBGFX_CONFIG_RENDERER_DIRECT3D11=1
  )

include_directories(
  third_party
  third_party/bx/include
  third_party/bgfx/include
  .build

  # XXX Windows only.
  third_party/bx/include/compat/msvc

  # XXX
  third_party/bgfx/examples/common
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vs_fontsdf.bin.h
  COMMAND ../third_party/bgfx/.build/win32_vs2012/bin/shadercRelease -i ../third_party/bgfx/src --type vertex --platform windows -p vs_4_0 -O 3 -f ../src/vs_fontsdf.sc --bin2c vs_fontsdf_dx11 -o ${CMAKE_CURRENT_BINARY_DIR}/vs_fontsdf.bin.h
  DEPENDS third_party/bgfx/.build/win32_vs2012/bin/shadercRelease.exe src/vs_fontsdf.sc
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fs_fontsdf.bin.h
  COMMAND ../third_party/bgfx/.build/win32_vs2012/bin/shadercRelease -i ../third_party/bgfx/src --type fragment --platform windows -p ps_4_0 -O 3 -f ../src/fs_fontsdf.sc --bin2c fs_fontsdf_dx11 -o ${CMAKE_CURRENT_BINARY_DIR}/fs_fontsdf.bin.h
  DEPENDS third_party/bgfx/.build/win32_vs2012/bin/shadercRelease.exe src/fs_fontsdf.sc
  )

add_executable(debugcanvas
  src/main.cc
  # XXX Windows only.
  src/entry_win.cc

  vs_fontsdf.bin.h
  fs_fontsdf.bin.h

  # bgfx example setup stuff, to be nuked.
  src/font_manager.cpp
  src/text_buffer_manager.cpp
  src/text_metrics.cpp
  src/utf8.cpp
  src/cube_atlas.cpp

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
