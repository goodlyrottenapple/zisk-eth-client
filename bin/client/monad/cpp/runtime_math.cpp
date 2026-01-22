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

#include "uint256.hpp"

// RISC-V implementation of monad_vm_runtime_mul that calls zisk's wmul256_c
#if defined(__riscv)

extern "C" void wmul256_c(uint64_t const* a, uint64_t const* b, uint64_t* result);

extern "C" void monad_vm_runtime_mul(
    monad::vm::runtime::uint256_t *result,
    monad::vm::runtime::uint256_t const *left,
    monad::vm::runtime::uint256_t const *right) noexcept
{
    wmul256_c(
        reinterpret_cast<uint64_t const*>(left),
        reinterpret_cast<uint64_t const*>(right),
        reinterpret_cast<uint64_t*>(result)
    );
}

#endif
