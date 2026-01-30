#include "test_vm.hpp"

#include "../evm/traits.hpp"

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
            auto& vm = reinterpret_cast<TestVM*>(c_vm)->vm_;
            std::span<uint8_t const> code_span{code, code_size};

            // Only EVMC_CANCUN is supported
            evmc::Result result;
            if (rev == EVMC_CANCUN) {
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_CANCUN>>(
                    host, ctx, msg, code_span);
            } else {
                // All other revisions are unsupported
                result = evmc::Result{EVMC_UNDEFINED_INSTRUCTION};
            }

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
