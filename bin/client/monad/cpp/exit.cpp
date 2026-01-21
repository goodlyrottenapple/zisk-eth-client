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

#include "runtime_types.hpp"
#include <csetjmp>

// Portable exit implementation using setjmp/longjmp for RISC-V
// Replaces the x86-specific assembly exit mechanism

namespace monad::vm::runtime
{
    void Context::stack_unwind [[noreturn]] () noexcept
    {
        is_stack_unwinding_active = true;
        result.status = StatusCode::Error;

        // Use longjmp to return to the execute() function
        if (exit_jmp_buf != nullptr) {
            longjmp(*exit_jmp_buf, 1);
        }
        // Should never reach here
        __builtin_unreachable();
    }

    void Context::exit [[noreturn]] (StatusCode code) noexcept
    {
        result.status = code;

        // Use longjmp to return to the execute() function
        if (exit_jmp_buf != nullptr) {
            longjmp(*exit_jmp_buf, 1);
        }
        // Should never reach here
        __builtin_unreachable();
    }
}

// C interface for compatibility with existing code
extern "C" void monad_vm_runtime_context_error_exit
    [[noreturn]] (monad::vm::runtime::Context *ctx)
{
    ctx->result.status = monad::vm::runtime::StatusCode::OutOfGas;

    if (ctx->exit_jmp_buf != nullptr) {
        longjmp(*ctx->exit_jmp_buf, 1);
    }
    // Should never reach here
    __builtin_unreachable();
}
