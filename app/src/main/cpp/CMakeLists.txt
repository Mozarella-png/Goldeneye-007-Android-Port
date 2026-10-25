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
  # Keep in sync if the root build's region table changes.
  set(ASSET_REGION "u")
  set(VERSION_IS_PAL 0)
  set(TARGET_IS_64BIT TRUE)
  set(REGION_DEFS
    -DVERSION_US -DLANG_US -DREFRESH_NTSC
    -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DBYTEMATCH)

  # Template variables the root CMakeLists.txt defines for port/include/*.in.
  # Values for the PC build come from its own logic; these are the Android
  # equivalents (NDK sysroot instead of the host gcc).
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
  # On Android these come from the NDK sysroot and the clang resource dir.
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=include
                  OUTPUT_VARIABLE _GE_CLANG_INC OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(PORT_HOST_STDDDEF_H "${_GE_CLANG_INC}/stddef.h")
  set(PORT_HOST_STRING_H  "${CMAKE_SYSROOT}/usr/include/string.h")
  set(PORT_HOST_SCHED_H   "${CMAKE_SYSROOT}/usr/include/sched.h")
  set(PORT_HOST_STDLIB_H  "${CMAKE_SYSROOT}/usr/include/stdlib.h")
  # Real zlib for rzdecomp.c. The NDK sysroot does not ship zlib.h, so CI fetches
  # the zlib source into app/src/main/cpp/zlib and it builds as a subdirectory.
  set(GE_ZLIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
  if(NOT EXISTS "${GE_ZLIB_SRC}/zlib.h")
    message(FATAL_ERROR "zlib source missing in app/src/main/cpp/zlib (CI fetches zlib v1.3.1 there)")
  endif()
  set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory(zlib)
  set(PORT_REAL_ZLIB_H "${GE_ZLIB_SRC}/zlib.h")
  set(GE_ZLIB_LIB zlibstatic)
  set(GE_ZLIB_INCLUDES "${GE_ZLIB_SRC}" "${CMAKE_CURRENT_BINARY_DIR}/zlib")

  # Generated from the root CMakeLists.txt: source lists (SRC_GAME, SRC_ENGINE,
  # SRC_ASSETS, SRC_LIBULTRA*, SRC_ROMASSETS, SRC_DRAM_SYMS), add_definitions,
  # add_compile_options, include_directories and the configure_file steps.
  include(${CMAKE_CURRENT_SOURCE_DIR}/port.cmake)

  add_definitions(-DPLATFORM_64BIT=1)

  # Port layer and fast3d, as the root build globs them.
  file(GLOB_RECURSE SRC_PORT     "${GE_ROOT}/port/src/*.c")
  file(GLOB_RECURSE SRC_PORT_CXX "${GE_ROOT}/port/src/*.cpp")
  file(GLOB_RECURSE SRC_FAST3D   "${GE_ROOT}/port/fast3d/*.cpp"
                                 "${GE_ROOT}/port/fast3d/glad/*.c")

  set(GE_DECOMP_C
    ${SRC_GAME} ${SRC_ENGINE} ${SRC_LIBAUDIO} ${SRC_LIBULTRARE_AUDIO}
    ${SRC_LIBULTRARE_DATA} ${SRC_LIBULTRARE_LIBC} ${SRC_ASSETS} ${SRC_GU}
    ${SRC_PORT}
  )
  # The decomp's `#define inherits struct` (src/bondtypes.h) embeds a base struct
  # only with a compiler extension: GCC's -fplan9-extensions (the PC build) or
  # -fms-extensions, which the root CMakeLists.txt's own comment notes is what
  # MinGW uses for the same embedding. Clang does not accept plan9 extensions,
  # so use -fms-extensions here. Applied per source file, not directory-wide,
  # so touch_layout.c and the C++ sources are unaffected.
  set_source_files_properties(${GE_DECOMP_C} PROPERTIES COMPILE_OPTIONS "-fms-extensions")

  list(APPEND GE_SOURCES
    ${SRC_GAME} ${SRC_ENGINE} ${SRC_ROMASSETS} ${SRC_DRAM_SYMS}
    ${SRC_PORT} ${SRC_PORT_CXX} ${SRC_FAST3D}
    ${SRC_LIBAUDIO} ${SRC_LIBULTRARE_AUDIO} ${SRC_LIBULTRARE_DATA}
    ${SRC_LIBULTRARE_LIBC} ${SRC_ASSETS} ${SRC_GU}
  )
endif()

add_library(main SHARED ${GE_SOURCES})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(GE_BUILD_PORT)
  target_include_directories(main PRIVATE ${GE_ZLIB_INCLUDES}
    ${GE_ROOT}/port/include ${GE_ROOT}/port/fast3d ${GE_ROOT}/include
    ${CMAKE_CURRENT_BINARY_DIR}/port/include)
endif()

# OpenGL ES 3 instead of desktop GL. The port's renderer (port/fast3d/
# gfx_opengl.cpp) still needs its GLES backend; see ANDROID-README.md.
if(GE_BUILD_PORT)
  target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m ${GE_ZLIB_LIB})
else()
  target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m z)
endif()
