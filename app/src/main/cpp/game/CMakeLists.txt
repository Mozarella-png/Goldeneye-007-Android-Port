cmake_minimum_required(VERSION 3.22.1)

# GoldenEye decomp sources (src/*), built as their own static library.
# port.cmake brings the include order the decomp needs (port/shim first, which
# shadows libc with the N64 shims). Those include directories and definitions
# are directory-scoped, so they do not reach the port target in the parent.
include(${CMAKE_CURRENT_SOURCE_DIR}/../port.cmake)

# Assembly files (no compile options): romassets and dram_syms.
set(GE_GAME_ASM ${SRC_ROMASSETS} ${SRC_DRAM_SYMS})

# C sources of the decomp.
set(GE_GAME_C
  ${SRC_GAME} ${SRC_ENGINE} ${SRC_LIBAUDIO} ${SRC_LIBULTRARE_AUDIO}
  ${SRC_LIBULTRARE_DATA} ${SRC_LIBULTRARE_LIBC} ${SRC_ASSETS} ${SRC_GU}
)

# Compiler workarounds for the decomp (see ANDROID-README.md):
#  - -fms-extensions: the `#define inherits struct` idiom (src/bondtypes.h)
#    embeds a base struct only with an extension. MinGW uses this flag for the
#    same embedding. Clang rejects -fplan9-extensions, so this one.
#  - -Wno-error=format-security: NDK defaults to -Werror=format-security. The
#    decomp passes game strings (langGet) as printf formats. The PC build only
#    warns, so this matches it.
set_source_files_properties(${GE_GAME_C} PROPERTIES
  COMPILE_OPTIONS "-fms-extensions;-Wno-error=format-security")

# The decomp selects its LP64 layouts with `#if defined(__x86_64__)`
# (music.c MUSIC_ALLOCATION_BYTES, objecthandler.c pool sizes). arm64 Android is
# LP64 little-endian with the same struct layout, so the macro is defined for
# the decomp sources only. Not for the port: system.c would then pull in x86
# intrinsics, and SDL and bionic must not see it.
set_source_files_properties(${GE_GAME_C} PROPERTIES
  COMPILE_DEFINITIONS "__x86_64__=1")

add_library(ge_game STATIC ${GE_GAME_C} ${GE_GAME_ASM})
