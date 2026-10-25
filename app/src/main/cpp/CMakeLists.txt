cmake_minimum_required(VERSION 3.22.1)
project(goldeneye007 C CXX)

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

# GE_BUILD_PORT=OFF (default): only the touch layer and SDL are compiled. This
# is the state CI verifies. Turn ON once the game sources are wired in (see
# ANDROID-README.md, step "Fuentes del juego"); it adds port/ to the build.
option(GE_BUILD_PORT "Compile the GoldenEye port sources (port/)" OFF)

set(GE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/touch_layout.c
)

if(GE_BUILD_PORT)
  file(GLOB GE_PORT_C   "${GE_ROOT}/port/src/*.c")
  file(GLOB GE_PORT_CXX "${GE_ROOT}/port/src/*.cpp"
                        "${GE_ROOT}/port/fast3d/*.cpp")
  list(APPEND GE_SOURCES ${GE_PORT_C} ${GE_PORT_CXX})
  message(STATUS "GE_BUILD_PORT=ON: compiling port sources from ${GE_ROOT}/port")
endif()

add_library(main SHARED ${GE_SOURCES})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(GE_BUILD_PORT)
  target_include_directories(main PRIVATE
    ${GE_ROOT}/port/include
    ${GE_ROOT}/port/fast3d
    ${GE_ROOT}/include
  )
endif()

# OpenGL ES 3 instead of desktop GL. The renderer still needs its GLES backend
# (see ANDROID-README.md); this line only links the library.
target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m z)
