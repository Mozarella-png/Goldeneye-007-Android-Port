cmake_minimum_required(VERSION 3.22.1)
project(goldeneye007 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# SDL2 is fetched into ./SDL by CI (see .github/workflows/android.yml).
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
  message(FATAL_ERROR "SDL2 source missing in app/src/main/cpp/SDL (CI copies it there)")
endif()
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Repo root is four levels up from this file (app/src/main/cpp).
set(GE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(GE_BIN  "${CMAKE_CURRENT_BINARY_DIR}")

# GE_BUILD_PORT=OFF (default): only the touch layer and SDL are compiled. This
# is the state the CI build-apk job verifies. -PgeBuildPort=true turns the
# port on (the build-port-apk job in CI does that, non-blocking).
option(GE_BUILD_PORT "Compile the GoldenEye port (port/ + game sources)" OFF)

set(GE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/touch_layout.c
)

if(GE_BUILD_PORT)
  message(STATUS "GE_BUILD_PORT=ON: building the port from ${GE_ROOT}")

  # Same values the PC build uses for ROMID=ntsc-final (CMakeLists.txt).
  set(ASSET_REGION "u")
  set(VERSION_IS_PAL 0)
  set(TARGET_IS_64BIT TRUE)
  set(REGION_DEFS
    -DVERSION_US -DLANG_US -DREFRESH_NTSC
    -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DBYTEMATCH)

  # Template variables the root CMakeLists.txt defines for port/include/*.in.
  set(TARGET_PLATFORM "${ANDROID_ABI}-android")
  set(ROMID "ntsc-final")
  set(VERSION_CODENAME "larkspur")
  set(GE007_VERSION "0.5.0")
  set(ORIGIN_URL "https://github.com/jkdansereau/goldeneye-pc-port")
  execute_process(COMMAND git -C "${GE_ROOT}" rev-parse --short HEAD
                  OUTPUT_VARIABLE VERSION_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
  if(NOT VERSION_HASH)
    set(VERSION_HASH "unknown")
  endif()
  # Host C headers that the port's hoststdlib/hoststring/... wrappers include.
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=include
                  OUTPUT_VARIABLE _GE_CLANG_INC OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(PORT_HOST_STDDDEF_H "${_GE_CLANG_INC}/stddef.h")
  set(PORT_HOST_STRING_H  "${CMAKE_SYSROOT}/usr/include/string.h")
  set(PORT_HOST_SCHED_H   "${CMAKE_SYSROOT}/usr/include/sched.h")
  set(PORT_HOST_STDLIB_H  "${CMAKE_SYSROOT}/usr/include/stdlib.h")

  # Real zlib for rzdecomp.c (CI fetches zlib v1.3.1 into app/src/main/cpp/zlib).
  set(GE_ZLIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
  if(NOT EXISTS "${GE_ZLIB_SRC}/zlib.h")
    message(FATAL_ERROR "zlib source missing in app/src/main/cpp/zlib (CI fetches zlib v1.3.1 there)")
  endif()
  set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory(zlib)
  set(PORT_REAL_ZLIB_H "${GE_ZLIB_SRC}/zlib.h")
  set(GE_ZLIB_LIB zlibstatic)
  set(GE_ZLIB_INCLUDES "${GE_ZLIB_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/zlib")

  # Decomp library (game/ subdirectory). It includes port.cmake, which
  # configures port/include/*.in into ${GE_BIN}/port/include. The port target
  # below depends on those generated headers, so this must come first.
  add_subdirectory(game)

  # Decomp headers the port needs, without the libc shims. Copied so they can be
  # the only shim-like directory on the port's include path.
  file(MAKE_DIRECTORY "${GE_BIN}/ge-port-core")
  file(COPY
    "${GE_ROOT}/port/shim/bondtypes.h"
    "${GE_ROOT}/port/shim/bondconstants.h"
    "${GE_ROOT}/port/shim/bondaicommands.h"
    "${GE_ROOT}/port/shim/R4300.h"
    "${GE_ROOT}/port/shim/CPPLib.h"
    DESTINATION "${GE_BIN}/ge-port-core")

  # Port layer and fast3d, as the root build globs them.
  file(GLOB_RECURSE SRC_PORT     "${GE_ROOT}/port/src/*.c")
  file(GLOB_RECURSE SRC_PORT_CXX "${GE_ROOT}/port/src/*.cpp")
  file(GLOB_RECURSE SRC_FAST3D   "${GE_ROOT}/port/fast3d/*.cpp"
                                 "${GE_ROOT}/port/fast3d/glad/*.c")

  list(APPEND GE_SOURCES ${SRC_PORT} ${SRC_PORT_CXX} ${SRC_FAST3D})
endif()

add_library(main SHARED ${GE_SOURCES})

if(GE_BUILD_PORT)
  # The port's own compile settings. Target-scoped, so they do not reach the
  # decomp library (which has its own directory with the shim order).
  target_compile_definitions(main PRIVATE
    PORT=1 _LANGUAGE_C=1 AVOID_UB=1 PAL=0 PLATFORM_64BIT=1)
  target_compile_options(main PRIVATE
    ${REGION_DEFS}
    -funsigned-char -fwrapv -fno-strict-aliasing
    $<$<COMPILE_LANGUAGE:C>:-fms-extensions>
    $<$<COMPILE_LANGUAGE:C>:-Wno-error=format-security>
    $<$<COMPILE_LANGUAGE:C>:-include;versioninfo.h>
    # libc comes from bionic first. The decomp's headers come after the system
    # headers, so <limits.h>, <stdlib.h>, <string.h>, <stddef.h> are bionic's.
    -idirafter "${GE_ROOT}/port/shim"
    -idirafter "${GE_ROOT}/include")
  target_include_directories(main PRIVATE
    "${GE_BIN}/ge-port-core"
    "${GE_ROOT}/port/include"
    "${GE_ROOT}"
    "${GE_ROOT}/src"
    "${GE_ROOT}/src/game"
    "${GE_ROOT}/src/libultra"
    "${GE_ROOT}/src/libultra/audio"
    "${GE_BIN}/port/include"
    "${GE_ROOT}/port/fast3d"
    ${GE_ZLIB_INCLUDES})
endif()

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# OpenGL ES 3 instead of desktop GL. The port's renderer (port/fast3d/
# gfx_opengl.cpp) still needs its GLES backend; see ANDROID-README.md.
if(GE_BUILD_PORT)
  target_link_libraries(main PRIVATE ge_game SDL2 GLESv3 EGL log android m ${GE_ZLIB_LIB})
else()
  target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m z)
endif()
