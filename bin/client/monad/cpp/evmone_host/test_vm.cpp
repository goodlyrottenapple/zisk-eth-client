#include "test_vm.hpp"

#include "evm/traits.hpp"

#include <evmc/evmc.hpp>

#include <cassert>
#include <span>

namespace evmone::test
{
    namespace
    {
        void destroy(evmc_vm* vm) noexcept
        {
            assert(vm != nullptr);
            delete reinterpret_cast<TestVM*>(vm);
        }

        constexpr evmc_capabilities_flagset get_capabilities(evmc_vm* /*vm*/) noexcept
        {
            return EVMC_CAPABILITY_EVM1;
        }

        evmc_set_option_result set_option(
            evmc_vm* /*vm*/, char const* /*name*/, char const* /*value*/) noexcept
        {
            return EVMC_SET_OPTION_INVALID_NAME;
        }

        evmc_result execute(
            evmc_vm* c_vm, evmc_host_interface const* host, evmc_host_context* ctx,
            evmc_revision rev, evmc_message const* msg, uint8_t const* code,
            size_t code_size) noexcept
        {
            auto& tvm = *reinterpret_cast<TestVM*>(c_vm);
            std::span<uint8_t const> code_span{code, code_size};

            // Only EVMC_CANCUN is supported
            if (rev != EVMC_CANCUN) {
                return evmc::Result{EVMC_UNDEFINED_INSTRUCTION}.release_raw();
            }

            // Save previous runtime context (non-null for nested calls)
            auto *const prev_rt_ctx = tvm.rt_ctx_;

            // Create runtime context from the EVMC message
            auto rt_ctx = monad::vm::runtime::Context::from(
                host, ctx, msg, code_span);
            tvm.rt_ctx_ = &rt_ctx;

            auto result = tvm.vm_.execute_bytecode_raw<monad::EvmTraits<EVMC_CANCUN>>(
                rt_ctx, code_span);

            // Propagate memory reallocation back to parent (or clean up
            // if this is the top-level call)
            rt_ctx.return_to<monad::EvmTraits<EVMC_CANCUN>>(prev_rt_ctx);
            tvm.rt_ctx_ = prev_rt_ctx;

            return result.release_raw();
        }

    }  // namespace

    TestVM::TestVM() noexcept
      : evmc_vm{
            .abi_version = EVMC_ABI_VERSION,
            .name = "monad-test-vm",
            .version = "0.1.0",
            .destroy = evmone::test::destroy,
            .execute = evmone::test::execute,
            .get_capabilities = evmone::test::get_capabilities,
            .set_option = evmone::test::set_option,
        }
    {
    }

}  // namespace evmone::test
