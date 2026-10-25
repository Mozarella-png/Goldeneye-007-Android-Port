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

  list(APPEND GE_SOURCES
    ${SRC_GAME} ${SRC_ENGINE} ${SRC_ROMASSETS} ${SRC_DRAM_SYMS}
    ${SRC_PORT} ${SRC_PORT_CXX} ${SRC_FAST3D}
    ${SRC_LIBAUDIO} ${SRC_LIBULTRARE_AUDIO} ${SRC_LIBULTRARE_DATA}
    ${SRC_LIBULTRARE_LIBC} ${SRC_ASSETS} ${SRC_GU}
  )
endif()

add_library(main SHARED ${GE_SOURCES})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# OpenGL ES 3 instead of desktop GL. The port's renderer (port/fast3d/
# gfx_opengl.cpp) still needs its GLES backend; see ANDROID-README.md.
target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m z)
