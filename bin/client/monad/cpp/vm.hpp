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

// #include <category/vm/code.hpp>
#include "evm/traits.hpp"
#include "host.hpp"
#include "interpreter/execute.hpp"
#include "runtime/allocator.hpp"


namespace monad::vm
{
    class VM
    {
        runtime::EvmStackAllocator stack_allocator_;

    public:
        explicit VM(
            std::size_t max_stack_cache_byte_size =
                runtime::EvmStackAllocator::DEFAULT_MAX_CACHE_BYTE_SIZE);

        /// Execute the bytecode `code` with interpreter.
        template <Traits traits>
        evmc::Result execute_bytecode(
            Host &host, evmc_message const *msg, std::span<uint8_t const> code);

        /// Execute on a pre-built runtime context (no stack unwind support).
        /// Caller is responsible for calling rt_ctx.return_to() afterwards.
        template <Traits traits>
        evmc::Result execute_bytecode_raw(
            runtime::Context &rt_ctx, std::span<uint8_t const> code);
    };
}
