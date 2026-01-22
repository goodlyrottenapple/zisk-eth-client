# RISC-V newlib C Library Sources

This directory contains source files from the newlib C library implementation for RISC-V architecture.

## Source Repository

These files were retrieved from the official riscv-newlib repository:

**Repository:** https://sourceware.org/git/newlib-cygwin.git
**Commit:** `4b940bb87e2e7aa524cbc73514bc19474650342e`
**Source Path:** `newlib/libc/machine/riscv/`
**Direct Link:** https://www.sourceware.org/git/?p=newlib-cygwin.git;a=tree;f=newlib/libc/machine/riscv;h=230fd864dd6fae42245c087e577d2dbc95102ccd;hb=4b940bb87e2e7aa524cbc73514bc19474650342e

## Why These Sources?

We use these newlib sources because:

1. **Linking to precompiled libc causes relocation errors**: Attempting to link with the precompiled `libc.a` from `/home/sam/.zisk/riscv/riscv64-unknown-elf/lib/libc.a` resulted in `relocation R_RISCV_HI20 out of range` errors because the library is too large for `mcmodel=medany` (±512KB address range limitation)
2. **No libc linking**: We cannot link with the full `libc.a` due to its size and the relocation constraints
3. **Tail call support**: We cannot use `mcmodel=large` because it breaks tail call optimization required by the `__attribute__((musttail))` in the EVM interpreter
4. **Authentic implementations**: Rather than custom stub implementations, we use the official SiFive/newlib implementations to ensure correctness and RISC-V ABI compliance

## Build Integration

These sources are compiled only for the RISC-V target and are not used in the x86 build. The assembly files are compiled with the RISC-V cross-compiler and linked into `libmonad_wrapper.a`.

See the parent `Makefile` for build rules.
