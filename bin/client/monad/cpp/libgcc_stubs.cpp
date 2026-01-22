// Copyright (C) 2025 Category Labs, Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdint>

// RISC-V libgcc function stubs
// These replace problematic libgcc functions that cause relocation errors
#if defined(__riscv)

// Stub for __assert_func to avoid linker errors
extern "C" void __assert_func(const char *file, int line, const char *func, const char *expr) noexcept
{
    // For RISC-V/zisk, we can't print, so just trap or loop
    while (1) { }
}

// Provide __clzdi2 to avoid pulling it from libgcc (which causes relocation errors)
// libgcc's version uses a lookup table (__clz_tab) that causes R_RISCV_HI20 relocations
// to be out of range. We use a binary search approach instead.
extern "C" int __clzdi2(uint64_t x) noexcept
{
    if (x == 0) return 64;

    int n = 0;
    if (x <= 0x00000000FFFFFFFF) { n += 32; x <<= 32; }
    if (x <= 0x0000FFFFFFFFFFFF) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFFFFFFFFFF) { n +=  8; x <<=  8; }
    if (x <= 0x0FFFFFFFFFFFFFFF) { n +=  4; x <<=  4; }
    if (x <= 0x3FFFFFFFFFFFFFFF) { n +=  2; x <<=  2; }
    if (x <= 0x7FFFFFFFFFFFFFFF) { n +=  1; }
    return n;
}

#endif
