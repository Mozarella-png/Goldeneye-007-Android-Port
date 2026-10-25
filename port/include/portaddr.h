#ifndef PORT_PORTADDR_H
#define PORT_PORTADDR_H

/* Stage B step 1 (STAGEB-ABI-DESIGN, D441 follow-on): the ONE place that
 * names the N64-DRAM arena windows. port/src/dram.c maps an 8 MB backing
 * store at BOTH views; everything that converts or range-checks an arena
 * address uses these instead of a literal, so moving the arena later (the
 * high-arena test flavour, macOS/ARM port_addr) is a change in one header.
 * Compile-time constants: no codegen change from the old literals. */
/* Each define is #ifndef-guarded so a -D on the command line can override it
 * (the D441 high-arena test flavour: GE_HIGHARENA_BASE=0x90000000 adds
 * -DPORT_DRAM_V1_BASE=0x90000000 for C, CXX and ASM). Values unchanged. */
#ifndef PORT_DRAM_V1_BASE
#define PORT_DRAM_V1_BASE  0x70000000UL  /* s32-safe "virtual" view (game RAM symbols) */
#endif
#ifndef PORT_DRAM_K0_BASE
#define PORT_DRAM_K0_BASE  0x80000000UL  /* KSEG0 mirror of V1 (same pages) */
#endif
/* N64 cart base: where romdata.c maps the .z64 and where the romassets_*.s
 * symbols point. PC keeps the N64 address. Android cannot use it: the ART
 * heap occupies 0x02000000-0x12000000 in the app process, so the ROM goes at
 * 0x40000000 instead. Changing this requires the matching romassets file:
 * port/src/romassets_android_<r>.s is generated for 0x40000000 by
 * tools_pc/android/rebase_romassets.py. Plain integer (no suffix) so the
 * assembler files can use it too. */
#ifndef PORT_CART_BASE
#if defined(__ANDROID__)
#define PORT_CART_BASE  0x40000000
#else
#define PORT_CART_BASE  0x10000000
#endif
#endif
#ifndef PORT_DRAM_SIZE
#define PORT_DRAM_SIZE     0x00800000UL  /* 8 MB, like an 8-MB-RAM N64 */
#endif

#endif /* PORT_PORTADDR_H */
