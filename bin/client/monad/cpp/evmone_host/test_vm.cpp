#include "test_vm.hpp"

#include "../evm/traits.hpp"

#include "../evmc/evmc.hpp"

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

            evmc::Result result;
            switch (rev) {
            case EVMC_FRONTIER:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_FRONTIER>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_HOMESTEAD:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_HOMESTEAD>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_TANGERINE_WHISTLE:
                result =
                    vm.execute_bytecode_raw<monad::EvmTraits<EVMC_TANGERINE_WHISTLE>>(
                        host, ctx, msg, code_span);
                break;
            case EVMC_SPURIOUS_DRAGON:
                result =
                    vm.execute_bytecode_raw<monad::EvmTraits<EVMC_SPURIOUS_DRAGON>>(
                        host, ctx, msg, code_span);
                break;
            case EVMC_BYZANTIUM:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_BYZANTIUM>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_CONSTANTINOPLE:
                result =
                    vm.execute_bytecode_raw<monad::EvmTraits<EVMC_CONSTANTINOPLE>>(
                        host, ctx, msg, code_span);
                break;
            case EVMC_PETERSBURG:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_PETERSBURG>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_ISTANBUL:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_ISTANBUL>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_BERLIN:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_BERLIN>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_LONDON:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_LONDON>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_PARIS:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_PARIS>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_SHANGHAI:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_SHANGHAI>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_CANCUN:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_CANCUN>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_PRAGUE:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_PRAGUE>>(
                    host, ctx, msg, code_span);
                break;
            case EVMC_OSAKA:
                result = vm.execute_bytecode_raw<monad::EvmTraits<EVMC_OSAKA>>(
                    host, ctx, msg, code_span);
                break;
            default:
                result = evmc::Result{EVMC_UNDEFINED_INSTRUCTION};
                break;
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
