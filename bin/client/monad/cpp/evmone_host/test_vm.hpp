#pragma once

#include "../vm.hpp"
#include <category/vm/runtime/types.hpp>

#include <evmc/evmc.h>

namespace evmone::test
{
    /// Test VM that wraps monad::vm::VM and provides EVMC interface.
    /// Maintains a runtime context pointer so nested calls (CALL/CREATE)
    /// can propagate memory reallocation back to the parent via return_to().
    class TestVM : public evmc_vm
    {
    public:
        TestVM() noexcept;

        monad::vm::VM vm_;
        monad::vm::runtime::Context *rt_ctx_ = nullptr;
    };
}  // namespace evmone::test
