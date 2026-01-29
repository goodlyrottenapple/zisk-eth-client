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
#if defined(__riscv)

#include <cstdint>

// RISC-V libgcc function stubs
// These replace problematic libgcc functions that cause relocation errors
// Provide __clzdi2 to avoid pulling it from libgcc (which causes relocation errors)
// libgcc's version uses a lookup table (__clz_tab) that causes R_RISCV_HI20 relocations
// to be out of range.
/*
 * By Henry S. Warren: http://www.hackersdelight.org/HDcode/divlu.c.txt
 * http://www.hackersdelight.org/permissions.htm:
 * "You are free to use, copy, and distribute any of the code on this web
 *  site, whether modified by you or not. You need not give attribution."
 *
 * Source: https://svn.python.org/projects/python/branches/py3k-cdecimal/Modules/cdecimal/typearith.h
 */
extern "C" int __clzdi2(uint64_t x) noexcept
{
	int n;

	if (x == 0) return(64);

	n = 0;
	if (x <= 0x00000000FFFFFFFF) {n = n +32; x = x <<32;}
	if (x <= 0x0000FFFFFFFFFFFF) {n = n +16; x = x <<16;}
	if (x <= 0x00FFFFFFFFFFFFFF) {n = n + 8; x = x << 8;}
	if (x <= 0x0FFFFFFFFFFFFFFF) {n = n + 4; x = x << 4;}
	if (x <= 0x3FFFFFFFFFFFFFFF) {n = n + 2; x = x << 2;}
	if (x <= 0x7FFFFFFFFFFFFFFF) {n = n + 1;}

	return n;
}

#endif
