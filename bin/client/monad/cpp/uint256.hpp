// Simplified uint256 wrapper for intx library
#pragma once

#include "intx/intx.hpp"
#include <cstring>
#include <algorithm>

namespace monad::vm::runtime
{
    using uint256_t = intx::uint256;

    // Extension methods for uint256_t to match the interface expected by monad runtime
    [[gnu::always_inline]]
    inline uint8_t const* as_bytes(uint256_t const& val) noexcept
    {
        return reinterpret_cast<uint8_t const*>(&val);
    }

    [[gnu::always_inline]]
    inline uint8_t* as_bytes(uint256_t& val) noexcept
    {
        return reinterpret_cast<uint8_t*>(&val);
    }

    [[gnu::always_inline]]
    inline void store_be(uint256_t const& val, uint8_t* dest) noexcept
    {
        // intx stores in native (little-endian) format, so we need to convert to big-endian
        uint256_t be = intx::bswap(val);
        ::std::memcpy(dest, &be, 32);
    }

#if defined(__riscv) && __riscv_xlen == 64
    // Zisk syscall FFI declarations
    extern "C" {
        struct SyscallAdd256Params {
            const uint64_t* a;  // &[u64; 4]
            const uint64_t* b;  // &[u64; 4]
            uint64_t cin;       // u64
            uint64_t* c;        // &mut [u64; 4]
        };
        uint64_t syscall_add256(SyscallAdd256Params* params);
    }

    // 256-bit addition using Zisk syscall for provable execution
    [[gnu::always_inline]]
    inline uint256_t add(const uint256_t& a, const uint256_t& b) noexcept
    {
        uint256_t result;
        SyscallAdd256Params params{&a[0], &b[0], 0, &result[0]};
        syscall_add256(&params);
        return result;
    }

    // 256-bit subtraction using Zisk syscall (via two's complement)
    [[gnu::always_inline]]
    inline uint256_t sub(const uint256_t& a, const uint256_t& b) noexcept
    {
        // x - y = x + (~y + 1)
        uint256_t b_complement = ~b;
        uint256_t result;
        SyscallAdd256Params params{&a[0], &b_complement[0], 1, &result[0]};
        syscall_add256(&params);
        return result;
    }
#else
    // Native x86 implementation using standard intx operators
    [[gnu::always_inline]]
    inline uint256_t add(const uint256_t& a, const uint256_t& b) noexcept
    {
        return a + b;
    }

    [[gnu::always_inline]]
    inline uint256_t sub(const uint256_t& a, const uint256_t& b) noexcept
    {
        return a - b;
    }
#endif
}
