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

#pragma once

#include "../evm/traits.hpp"
#include "types.hpp"
#include "uint256.hpp"

#include "../evmc/evmc.hpp"

// It is assumed that if the `result` pointer overlaps with `left` and/or
// `right`, then `result` pointer is equal to `left` and/or `right`.
extern "C" void monad_vm_runtime_mul_192(
    monad::vm::runtime::uint256_t *result,
    monad::vm::runtime::uint256_t const *left,
    monad::vm::runtime::uint256_t const *right) noexcept;

// It is assumed that if the `result` pointer overlaps with `left` and/or
// `right`, then `result` pointer is equal to `left` and/or `right`.
extern "C" void monad_vm_runtime_mul(
    monad::vm::runtime::uint256_t *result,
    monad::vm::runtime::uint256_t const *left,
    monad::vm::runtime::uint256_t const *right) noexcept;

// RISC-V zisk syscall declarations
#if defined(__riscv)
extern "C" void divrem256_c(uint64_t const* a, uint64_t const* b, uint64_t* q, uint64_t* r);
extern "C" void addmod256_c(uint64_t const* a, uint64_t const* b, uint64_t const* m, uint64_t* result);
extern "C" void mulmod256_c(uint64_t const* a, uint64_t const* b, uint64_t const* m, uint64_t* result);
extern "C" void wpow256_c(uint64_t const* a, uint64_t const* exp, uint64_t* result);
#endif

namespace monad::vm::runtime
{
    constexpr void (*mul)(
        uint256_t *, uint256_t const *,
        uint256_t const *) noexcept = monad_vm_runtime_mul;

    // Division: use zisk divrem256_c on RISC-V, intx on x86
    inline void udiv(
        uint256_t *result_ptr, uint256_t const *a_ptr,
        uint256_t const *b_ptr) noexcept
    {
#if defined(__riscv)
        uint256_t remainder;
        divrem256_c(
            reinterpret_cast<uint64_t const*>(a_ptr),
            reinterpret_cast<uint64_t const*>(b_ptr),
            reinterpret_cast<uint64_t*>(result_ptr),
            reinterpret_cast<uint64_t*>(&remainder)
        );
#else
        if (*b_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = *a_ptr / *b_ptr;
#endif
    }

    // Signed division: stub for RISC-V, intx sdivrem for x86
    inline void sdiv(
        uint256_t *result_ptr, uint256_t const *a_ptr,
        uint256_t const *b_ptr) noexcept
    {

        if (*b_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = sdivrem(*a_ptr, *b_ptr).quot;
    }

    // Modulo: use zisk divrem256_c on RISC-V, intx on x86
    inline void umod(
        uint256_t *result_ptr, uint256_t const *a_ptr,
        uint256_t const *b_ptr) noexcept
    {
#if defined(__riscv)
        uint256_t quotient;
        divrem256_c(
            reinterpret_cast<uint64_t const*>(a_ptr),
            reinterpret_cast<uint64_t const*>(b_ptr),
            reinterpret_cast<uint64_t*>(&quotient),
            reinterpret_cast<uint64_t*>(result_ptr)
        );
#else
        if (*b_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = *a_ptr % *b_ptr;
#endif
    }

    // Signed modulo: stub for RISC-V, intx sdivrem for x86
    inline void smod(
        uint256_t *result_ptr, uint256_t const *a_ptr,
        uint256_t const *b_ptr) noexcept
    {
        if (*b_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = sdivrem(*a_ptr, *b_ptr).rem;
    }

    // Addmod: use zisk addmod256_c on RISC-V, intx on x86
    inline void addmod(
        uint256_t *result_ptr, uint256_t const *a_ptr, uint256_t const *b_ptr,
        uint256_t const *n_ptr) noexcept
    {
#if defined(__riscv)
        addmod256_c(
            reinterpret_cast<uint64_t const*>(a_ptr),
            reinterpret_cast<uint64_t const*>(b_ptr),
            reinterpret_cast<uint64_t const*>(n_ptr),
            reinterpret_cast<uint64_t*>(result_ptr)
        );
#else
        if (*n_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = intx::addmod(*a_ptr, *b_ptr, *n_ptr);
#endif
    }

    // Mulmod: use zisk mulmod256_c on RISC-V, intx on x86
    inline void mulmod(
        uint256_t *result_ptr, uint256_t const *a_ptr, uint256_t const *b_ptr,
        uint256_t const *n_ptr) noexcept
    {
#if defined(__riscv)
        mulmod256_c(
            reinterpret_cast<uint64_t const*>(a_ptr),
            reinterpret_cast<uint64_t const*>(b_ptr),
            reinterpret_cast<uint64_t const*>(n_ptr),
            reinterpret_cast<uint64_t*>(result_ptr)
        );
#else
        if (*n_ptr == 0) {
            *result_ptr = 0;
            return;
        }
        *result_ptr = intx::mulmod(*a_ptr, *b_ptr, *n_ptr);
#endif
    }

    // Count significant bytes in uint256_t
    // Uses intx for both x86 and RISC-V (we provide __clzdi2 for RISC-V)
    [[gnu::always_inline]]
    inline constexpr unsigned count_significant_bytes(uint256_t const& x) noexcept
    {
        return intx::count_significant_bytes(x);
    }

    // Gas cost multiplier for EXP opcode
    template <Traits traits>
    [[gnu::always_inline]]
    inline constexpr uint32_t exp_dynamic_gas_cost_multiplier() noexcept
    {
        if (traits::evm_rev() >= EVMC_SPURIOUS_DRAGON) {
            return 50;
        }
        else {
            return 10;
        }
    }

    // Exponentiation: use zisk wpow256_c on RISC-V, intx on x86
    template <Traits traits>
    inline void exp(Context *ctx, uint256_t *result_ptr, uint256_t const *a_ptr,
        uint256_t const *exponent_ptr) noexcept
    {
        auto exponent_byte_size = count_significant_bytes(*exponent_ptr);
        auto exponent_cost = exp_dynamic_gas_cost_multiplier<traits>();
        ctx->deduct_gas(exponent_byte_size * exponent_cost);

#if defined(__riscv)
        wpow256_c(
            reinterpret_cast<uint64_t const*>(a_ptr),
            reinterpret_cast<uint64_t const*>(exponent_ptr),
            reinterpret_cast<uint64_t*>(result_ptr)
        );
#else
        *result_ptr = intx::exp(*a_ptr, *exponent_ptr);
#endif
    }
}
