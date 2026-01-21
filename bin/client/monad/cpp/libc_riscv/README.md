# RISC-V newlib C Library Sources

This directory contains source files from the newlib C library implementation for RISC-V architecture.

## Source Repository

These files were retrieved from the official riscv-newlib repository:

**Repository:** https://github.com/riscvarchive/riscv-newlib
**Commit:** `f289cef6be67da67b2d97a47d6576fa7e6b4c858`
**Source Path:** `newlib/libc/machine/riscv/`
**Direct Link:** https://github.com/riscvarchive/riscv-newlib/tree/f289cef6be67da67b2d97a47d6576fa7e6b4c858/newlib/libc/machine/riscv

## License

Copyright (c) 2017 SiFive Inc. All rights reserved.

This copyrighted material is made available to anyone wishing to use, modify, copy, or redistribute it subject to the terms and conditions of the FreeBSD License. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY expressed or implied, including the implied warranties of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

A copy of this license is available at http://www.opensource.org/licenses.

## Files Included

### Assembly Files (.S)
- `setjmp.S` - setjmp/longjmp implementation for non-local control flow
- `memcpy-asm.S` - Optimized memory copy
- `memmove.S` - Optimized memory move with overlap handling
- `memset.S` - Optimized memory initialization
- `strcmp.S` - Optimized string comparison

### C Files (.c)
- `ffs.c` - Find first set bit operation
- `ieeefp.c` - IEEE floating-point handling
- `memcpy.c` - Memory copy wrapper
- `memmove-stub.c` - Memory move stub
- `strcpy.c` - String copy
- `strlen.c` - String length

### Header Files (sys/)
- `sys/asm.h` - Assembly macros for 32/64-bit compatibility
- `sys/fenv.h` - Floating-point environment
- `sys/string.h` - String operation declarations

## Why These Sources?

We use these newlib sources because:

1. **Linking to precompiled libc causes relocation errors**: Attempting to link with the precompiled `libc.a` from `/home/sam/.zisk/riscv/riscv64-unknown-elf/lib/libc.a` resulted in `relocation R_RISCV_HI20 out of range` errors because the library is too large for `mcmodel=medany` (±512KB address range limitation)
2. **No libc linking**: We cannot link with the full `libc.a` due to its size and the relocation constraints
3. **Tail call support**: We cannot use `mcmodel=large` because it breaks tail call optimization required by the `__attribute__((musttail))` in the EVM interpreter
4. **Authentic implementations**: Rather than custom stub implementations, we use the official SiFive/newlib implementations to ensure correctness and RISC-V ABI compliance

## Build Integration

These sources are compiled only for the RISC-V target and are not used in the x86 build. The assembly files are compiled with the RISC-V cross-compiler and linked into `libmonad_wrapper.a`.

See the parent `Makefile` for build rules.
