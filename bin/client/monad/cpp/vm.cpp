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

// #include <category/vm/code.hpp>
#include "evm/explicit_traits.hpp"
#include "evm/traits.hpp"
#include "host.hpp"
#include "runtime/allocator.hpp"
#include "runtime/types.hpp"
#include "vm.hpp"

#include <evmc/evmc.h>
#include <evmc/evmc.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace monad::vm
{
    VM::VM(std::size_t max_stack_cache)
        : stack_allocator_{max_stack_cache}
    {
    }

    template <Traits traits>
    evmc::Result VM::execute_bytecode(
        Host &host, evmc_message const *msg, std::span<uint8_t const> code)
    {
        auto const *const host_itf = &host.get_interface();
        auto *const host_ctx = host.to_context();
        auto rt_ctx = runtime::Context::from(host_itf, host_ctx, msg, code);

        // Install new runtime context:
        auto *const prev_rt_ctx = host.set_runtime_context(&rt_ctx);

        auto result = execute_bytecode_impl<traits>(rt_ctx, code);

        rt_ctx.return_to<traits>(prev_rt_ctx);

        // Re-install previous runtime context:
        (void)host.set_runtime_context(prev_rt_ctx);

        host.rethrow_on_active_exception();

        return result;
    }

    // Explicit instantiation for EVMC_CANCUN only
    template evmc::Result VM::execute_bytecode<EvmTraits<EVMC_CANCUN>>(
        Host &, evmc_message const *, std::span<uint8_t const>);

    template <Traits traits>
    evmc::Result VM::execute_bytecode_raw(
        evmc_host_interface const *host, evmc_host_context *host_ctx,
        evmc_message const *msg, std::span<uint8_t const> code)
    {
        auto rt_ctx = runtime::Context::from(host, host_ctx, msg, code);
        return execute_bytecode_impl<traits>(rt_ctx, code);
    }

    // Explicit instantiation for EVMC_CANCUN only
    template evmc::Result VM::execute_bytecode_raw<EvmTraits<EVMC_CANCUN>>(
        evmc_host_interface const *, evmc_host_context *,
        evmc_message const *, std::span<uint8_t const>);

    template <Traits traits>
    evmc::Result VM::execute_bytecode_impl(
        runtime::Context &rt_ctx, std::span<uint8_t const> code)
    {
        auto const stack_ptr = stack_allocator_.allocate();
        interpreter::execute<traits>(rt_ctx, monad::vm::interpreter::Intercode{code}, stack_ptr.get());

        return rt_ctx.copy_to_evmc_result<traits>();
    }

    // Explicit instantiation for EVMC_CANCUN only
    template evmc::Result VM::execute_bytecode_impl<EvmTraits<EVMC_CANCUN>>(
        runtime::Context &, std::span<uint8_t const>);
}
