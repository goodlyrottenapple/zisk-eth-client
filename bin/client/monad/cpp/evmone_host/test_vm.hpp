#pragma once

#include "../vm.hpp"

#include "../evmc/evmc.h"

namespace evmone::test
{
    /// Test VM that wraps monad::vm::VM and provides EVMC interface
    class TestVM : public evmc_vm
    {
    public:
        TestVM() noexcept;

        monad::vm::VM vm_;
    };
}  // namespace evmone::test
