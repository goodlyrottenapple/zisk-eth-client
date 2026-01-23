// Simplified uint256 wrapper for intx library
#pragma once

#include "intx/intx.hpp"
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
}
