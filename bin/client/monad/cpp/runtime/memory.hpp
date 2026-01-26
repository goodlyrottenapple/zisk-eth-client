// Copyright (C) 2025 Category Labs, Inc.
//
// Simplified memory operations for Monad EVM

#pragma once

#include "types.hpp"
#include "uint256.hpp"

namespace monad::vm::runtime
{
    inline void mstore(
        Context *ctx, uint256_t const *offset_ptr, uint256_t const *value_ptr)
    {
        auto offset = ctx->get_memory_offset(*offset_ptr);
        ctx->expand_memory(offset + bin<32>);
        store_be(*value_ptr, ctx->memory.data + *offset);
    }
}
