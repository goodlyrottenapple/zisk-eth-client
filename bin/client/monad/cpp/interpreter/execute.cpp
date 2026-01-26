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

#include "../evm/traits.hpp"
#include "instruction_table.hpp"
#include "intercode.hpp"
#include "../runtime/types.hpp"
#include "../runtime/uint256.hpp"
#include "../utils_traits.hpp"

#include <cstdint>
#include <csetjmp>

extern "C" void monad_vm_interpreter_trampoline(
    void *, ::monad::vm::runtime::Context *,
    ::monad::vm::interpreter::Intercode const *,
    ::monad::vm::runtime::uint256_t *, void *);

namespace monad::vm::interpreter
{
    namespace
    {
        template <Traits traits>
        void core_loop(
            void *, runtime::Context *ctx, Intercode const *analysis,
            runtime::uint256_t *stack_ptr, void *)
        {
            static_assert(
                utils::same_signature(
                    monad_vm_interpreter_trampoline, core_loop<traits>),
                "Interpreter core loop and trampoline signatures must be "
                "identical");
#if defined(__riscv)
            // Use setjmp to save the execution context for exit
            // This replaces the x86 trampoline's stack pointer saving
            jmp_buf exit_jmp;

            if (setjmp(exit_jmp)) {
                // If setjmp(exit_jmp) != 0, we longjmp'd back here from ctx.exit()
                // The result status has already been set
                return;
            }

            // First time through - set up exit mechanism and execute
            ctx->exit_stack_ptr = &exit_jmp;
#endif
            auto *const stack_top = stack_ptr - 1;
            auto const *const stack_bottom = stack_top;
            auto const *const instr_ptr = analysis->code();
            auto const gas_remaining = ctx->gas_remaining;

            if constexpr (debug_enabled) {
                trace(*analysis, gas_remaining, instr_ptr);
            }
            instruction_table<traits>[*instr_ptr](
                *ctx,
                *analysis,
                stack_bottom,
                stack_top,
                gas_remaining,
                instr_ptr);
        }
    }

    template <Traits traits>
    void execute(
        runtime::Context &ctx, Intercode const &analysis,
        std::uint8_t *stack_ptr)
    {
#if defined(__riscv)
        core_loop<traits>(nullptr,
            &ctx,
            &analysis,
            reinterpret_cast<runtime::uint256_t *>(stack_ptr), nullptr);
#else
        monad_vm_interpreter_trampoline(
            static_cast<void *>(&ctx.exit_stack_ptr),
            &ctx,
            &analysis,
            reinterpret_cast<runtime::uint256_t *>(stack_ptr),
            reinterpret_cast<void *>(core_loop<traits>));
#endif
    }

    // Explicit instantiation for MONAD_ZERO
    template void execute<MonadTraits<MONAD_ZERO>>(
        runtime::Context &, Intercode const &, std::uint8_t *);
}
