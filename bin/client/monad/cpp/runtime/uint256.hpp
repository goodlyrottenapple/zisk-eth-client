// Simplified uint256 wrapper for intx library
#pragma once

#include "../intx/intx.hpp"
#include "../evmc/evmc.hpp"
#include "../core/assert.h"
#include <cstring>
#include <algorithm>

namespace monad::vm::runtime
{
    using uint256_t = intx::uint256;

    uint256_t signextend(uint256_t const &byte_index, uint256_t const &x);
    uint256_t byte(uint256_t const &byte_index, uint256_t const &x);
    uint256_t sar(uint256_t const &shift_index, uint256_t const &x);
    uint256_t countr_zero(uint256_t const &x);

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

    [[gnu::always_inline]]
    inline uint256_t load_be(uint8_t const* src) noexcept
    {
        // Load big-endian bytes and convert to intx's native (little-endian) format
        uint256_t be;
        ::std::memcpy(&be, src, 32);
        return intx::bswap(be);
    }

    static_assert(sizeof(evmc_address) == 20);
    static_assert(sizeof(evmc_bytes32) == 32);
    static_assert(sizeof(uint256_t) == 32);

    [[gnu::always_inline]]
    inline uint256_t
    uint256_load_bounded_le(std::uint8_t const *bytes, std::int64_t max_len)
    {
        if (MONAD_VM_LIKELY(max_len >= 32)) {
            // Load full 32 bytes in little-endian (native intx format)
            uint256_t result;
            ::std::memcpy(&result, bytes, 32);
            return result;
        }
        // Load bounded bytes (less than 32), zero-fill the rest
        uint256_t result = 0;
        ::std::memcpy(&result, bytes, static_cast<std::size_t>(max_len));
        return result;
    }

    [[gnu::always_inline]]
    inline uint256_t
    uint256_load_bounded_be(std::uint8_t const *bytes, std::int64_t max_len)
    {
        if (MONAD_VM_LIKELY(max_len >= 32)) {
            // Load full 32 bytes in big-endian
            return load_be(bytes);
        }
        // Load bounded bytes in big-endian
        uint256_t result = 0;
        auto* dest = as_bytes(result);
        ::std::memcpy(const_cast<uint8_t*>(dest) + (32 - max_len), bytes, static_cast<std::size_t>(max_len));
        return intx::bswap(result);
    }

    [[gnu::always_inline]]
    inline evmc::bytes32 bytes32_from_uint256(uint256_t const &x)
    {
        evmc_bytes32 ret;
        store_be(x, ret.bytes);
        return ret;
    }

    [[gnu::always_inline]]
    inline evmc::address address_from_uint256(uint256_t const &x)
    {
        auto const *bytes = as_bytes(x);

        std::uint64_t t2;
        std::memcpy(&t2, bytes, 8);
        t2 = std::byteswap(t2);

        std::uint64_t t1;
        std::memcpy(&t1, bytes + 8, 8);
        t1 = std::byteswap(t1);

        std::uint32_t t0;
        std::memcpy(&t0, bytes + 16, 4);
        t0 = std::byteswap(t0);

        evmc_address ret;
        std::memcpy(ret.bytes, &t0, 4);
        std::memcpy(ret.bytes + 4, &t1, 8);
        std::memcpy(ret.bytes + 12, &t2, 8);
        return ret;
    }

    [[gnu::always_inline]]
    inline uint256_t uint256_from_bytes32(evmc::bytes32 const &x)
    {
        return load_be(x.bytes);
    }

    [[gnu::always_inline]]
    inline uint256_t uint256_from_address(evmc::address const &addr)
    {
        std::uint32_t t2;
        std::memcpy(&t2, addr.bytes, 4);
        t2 = std::byteswap(t2);

        std::uint64_t t1;
        std::memcpy(&t1, addr.bytes + 4, 8);
        t1 = std::byteswap(t1);

        std::uint64_t t0;
        std::memcpy(&t0, addr.bytes + 12, 8);
        t0 = std::byteswap(t0);

        alignas(uint256_t) uint8_t ret[32];
        std::memcpy(ret, &t0, 8);
        std::memcpy(ret + 8, &t1, 8);
        std::memcpy(ret + 16, &t2, 4);
        std::memset(ret + 20, 0, 12);
        return std::bit_cast<uint256_t>(ret);
    }

    template <std::uint64_t N>
        requires(N < 64)
    [[gnu::always_inline]]
    constexpr bool is_bounded_by_bits(uint256_t const &x)
    {
        static constexpr std::uint64_t mask = ~((std::uint64_t{1} << N) - 1);
        return ((x[0] & mask) | x[1] | x[2] | x[3]) == 0;
    }

    template <typename T>
    [[gnu::always_inline]]
    constexpr T clamp_cast(uint256_t const &x) noexcept
    {
        return is_bounded_by_bits<std::numeric_limits<T>::digits>(x)
                   ? static_cast<T>(x)
                   : std::numeric_limits<T>::max();
    }
}
