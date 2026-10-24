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

# Port layer, touch overlay and shared headers.
# TODO(port): game sources (src/game, src/libultra, rsp-free engine files) are
# listed in the root CMakeLists.txt as SRC_GAME / SRC_ENGINE / SRC_LIBULTRA*.
# They must be exported to a shared include so both builds use one list.
# Until then this target only compiles the layers below.
file(GLOB PORT_C   "${CMAKE_CURRENT_SOURCE_DIR}/../../../../port/src/*.c")
file(GLOB PORT_CXX "${CMAKE_CURRENT_SOURCE_DIR}/../../../../port/src/*.cpp"
                   "${CMAKE_CURRENT_SOURCE_DIR}/../../../../port/fast3d/*.cpp")

add_library(main SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/touch_layout.c
    ${CMAKE_CURRENT_SOURCE_DIR}/android_main.cpp
    ${PORT_C}
    ${PORT_CXX}
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../port/fast3d
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)

# OpenGL ES 3 instead of desktop GL (see docs/android.md: the renderer still needs
# its GLES backend; this line only links the library).
target_link_libraries(main PRIVATE SDL2 GLESv3 EGL log android m z)
