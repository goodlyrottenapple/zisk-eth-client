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
}
