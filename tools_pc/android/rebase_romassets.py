#!/usr/bin/env python3
"""Rebase port/src/romassets_<r>.s to another cart base for the Android build.

On Android the ART heap occupies 0x02000000-0x12000000 in the app process, so
the PC cart base 0x10000000 cannot be mapped there (see the GoldenEye crash log
in ANDROID-README). This writes port/src/romassets_android_<r>.s with every
cart address moved by (NEW_BASE - 0x10000000). romdata.c maps the ROM at the
same PORT_CART_BASE (port/include/portaddr.h), so the two must agree.

Usage: python3 tools_pc/android/rebase_romassets.py <u|e|j> [NEW_BASE_HEX]
Default NEW_BASE_HEX is 0x40000000 (matches portaddr.h for __ANDROID__).
Only the 0x10000000-0x10FFFFFF window is rewritten; other constants are left.
"""
import os
import re
import sys

OLD_BASE = 0x10000000
SPAN = 0x01000000
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def rebase(text, new_base):
    def repl(m):
        v = int(m.group(0), 16)
        if OLD_BASE <= v < OLD_BASE + SPAN:
            return "0x%08X" % (new_base + (v - OLD_BASE))
        return m.group(0)
    return re.sub(r"0x[0-9A-Fa-f]{8}\b", repl, text)


def main():
    if len(sys.argv) not in (2, 3) or sys.argv[1] not in ("u", "e", "j"):
        sys.exit("usage: rebase_romassets.py <u|e|j> [NEW_BASE_HEX]")
    region = sys.argv[1]
    new_base = int(sys.argv[2], 16) if len(sys.argv) == 3 else 0x40000000
    src = os.path.join(ROOT, "port", "src", f"romassets_{region}.s")
    dst = os.path.join(ROOT, "port", "src", f"romassets_android_{region}.s")
    with open(src, encoding="utf-8") as f:
        body = rebase(f.read(), new_base)
    header = (
        f"# Android variant: cart base 0x{new_base:08X}. GENERATED from "
        f"romassets_{region}.s by\n# tools_pc/android/rebase_romassets.py. Do not edit.\n"
    )
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(header + body)
    print(f"wrote {os.path.relpath(dst, ROOT)} (cart base 0x{new_base:08X})")


if __name__ == "__main__":
    main()
