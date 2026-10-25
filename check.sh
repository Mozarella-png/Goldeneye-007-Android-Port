#!/usr/bin/env bash
# Host syntax check of port/ sources, mirroring port.cmake flags (US region, 64-bit).
set -u
REPO=/home/claude/Goldeneye-007-Android-Port
SDL=/home/claude/sdl_dl/SDL/include
OUT=/home/claude/syncheck
GEN=$OUT/gen/port/include
rm -rf "$OUT/gen" && mkdir -p "$GEN"
STDDEF=$(gcc -print-file-name=include)/stddef.h
sub() { sed -e "s|@PORT_HOST_SCHED_H@|/usr/include/sched.h|" \
  -e "s|@PORT_HOST_STDDDEF_H@|$STDDEF|" -e "s|@PORT_HOST_STDLIB_H@|/usr/include/stdlib.h|" \
  -e "s|@PORT_HOST_STRING_H@|/usr/include/string.h|" -e "s|@PORT_REAL_ZLIB_H@|/usr/include/zlib.h|" \
  -e "s|@VERSION_HASH@|syncheck|" -e "s|@GE007_VERSION@|0.5.0|" -e "s|@VERSION_CODENAME@|check|" \
  -e "s|@ORIGIN_URL@||" -e "s|@TARGET_PLATFORM@|linux|" -e "s|@ROMID@|ntsc-final|" \
  -e "s|@VERSION_IS_PAL@|0|" "$1" > "$2"; }
for f in "$REPO"/port/include/*.in; do sub "$f" "$GEN/$(basename "${f%.in}")"; done
COMMON="-fsyntax-only -DPORT=1 -D__ANDROID__=1 -D_LANGUAGE_C=1 -DAVOID_UB=1 -DPAL=0 -DPLATFORM_64BIT=1 \
 -DVERSION_US -DLANG_US -DREFRESH_NTSC -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DBYTEMATCH \
 -fno-strict-aliasing -funsigned-char -fwrapv -fno-stack-protector -fno-omit-frame-pointer \
 -include versioninfo.h -Wall -Wno-address -Wno-misleading-indentation -Wno-missing-braces \
 -Wno-multichar -Wno-tautological-compare -Wno-unused-but-set-variable -Wno-unused-value \
 -Wno-unused-variable -Wno-error=int-conversion -Wno-error=incompatible-pointer-types \
 -Wno-error=implicit-function-declaration -Wno-pointer-sign \
 -I$REPO/port/shim -I$REPO -I$REPO/include -I$REPO/include/PR -I$REPO/src -I$REPO/src/game \
 -I$REPO/src/libultra -I$REPO/src/libultra/audio -I$REPO/port/include -I$GEN"
cd "$REPO"
report=$OUT/report.txt; : > "$report"
fail=0; total=0
for f in port/src/*.c port/fast3d/glad/glad.c; do
  total=$((total+1)); log=$OUT/logs/$(echo "$f" | tr / _).log; mkdir -p "$OUT/logs"
  if gcc $COMMON -I"$SDL" "$f" > "$log" 2>&1; then echo "OK   $f" >> "$report"
  else n=$(grep -c 'error:' "$log"); echo "FAIL $f ($n errors)" >> "$report"; fail=$((fail+1)); fi
done
for f in port/fast3d/*.cpp; do
  total=$((total+1)); log=$OUT/logs/$(echo "$f" | tr / _).log; mkdir -p "$OUT/logs"
  if g++ $COMMON -std=gnu++20 -Wno-unused-function -I"$SDL" -Iport/fast3d -I"$REPO/port/fast3d/glad" "$f" > "$log" 2>&1; then echo "OK   $f" >> "$report"
  else n=$(grep -c 'error:' "$log"); echo "FAIL $f ($n errors)" >> "$report"; fail=$((fail+1)); fi
done
echo "total=$total failed=$fail" >> "$report"
cat "$report"
