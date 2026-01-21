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

#include "traits.hpp"
#include "instruction_table.hpp"
#include "intercode.hpp"
#include "runtime_types.hpp"
#include "uint256.hpp"

#include <cstdint>
#include <csetjmp>

// Portable implementation using setjmp/longjmp for RISC-V
// Replaces the x86-specific assembly trampoline

namespace monad::vm::interpreter
{
    template <Traits traits>
    void execute(
        runtime::Context &ctx, Intercode const &analysis,
        std::uint8_t *stack_ptr)
    {
        // Use setjmp to save the execution context for exit
        // This replaces the x86 trampoline's stack pointer saving
        jmp_buf exit_jmp;
        int exit_code = setjmp(exit_jmp);

        if (exit_code == 0) {
            // First time through - set up exit mechanism and execute
            ctx.exit_jmp_buf = &exit_jmp;

            auto *const stack_top = reinterpret_cast<runtime::uint256_t *>(stack_ptr) - 1;
            auto const *const stack_bottom = stack_top;
            auto const *const instr_ptr = analysis.code();
            auto const gas_remaining = ctx.gas_remaining;

            // Call the first instruction to start execution
            instruction_table<traits>[*instr_ptr](
                ctx,
                analysis,
                stack_bottom,
                stack_top,
                gas_remaining,
                instr_ptr);

            // Should not reach here - execution should exit via ctx.exit()
            ctx.result.status = runtime::StatusCode::Error;
        }
        // If exit_code != 0, we longjmp'd back here from ctx.exit()
        // The result status has already been set
    }

    // Explicit instantiation for MONAD_ZERO
    template void execute<MonadTraits<MONAD_ZERO>>(
        runtime::Context &, Intercode const &, std::uint8_t *);
}
