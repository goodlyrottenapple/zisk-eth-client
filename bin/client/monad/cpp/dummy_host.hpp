// Copyright (C) 2025 - Minimal EVMC Host implementation for testing
// Provides a minimal implementation of the EVMC host interface with dummy values

#pragma once

#include "evmc/evmc.hpp"
#include <cstring>

namespace monad::vm::runtime
{

/// Minimal dummy host implementation for testing
/// All methods return dummy/zero values
class DummyHost : public evmc::Host
{
public:
    bool account_exists(const evmc::address& addr) const noexcept override
    {
        (void)addr;
        return false;  // No accounts exist in stub
    }

    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept override
    {
        (void)addr;
        (void)key;
        return evmc::bytes32{};  // Return zero
    }

    evmc_storage_status set_storage(const evmc::address& addr,
                                    const evmc::bytes32& key,
                                    const evmc::bytes32& value) noexcept override
    {
        (void)addr;
        (void)key;
        (void)value;
        return EVMC_STORAGE_ASSIGNED;  // Pretend it worked
    }

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept override
    {
        (void)addr;
        return evmc::uint256be{};  // Return zero balance
    }

    size_t get_code_size(const evmc::address& addr) const noexcept override
    {
        (void)addr;
        return 0;  // No code
    }

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept override
    {
        (void)addr;
        return evmc::bytes32{};  // Return zero hash
    }

    size_t copy_code(const evmc::address& addr,
                     size_t code_offset,
                     uint8_t* buffer_data,
                     size_t buffer_size) const noexcept override
    {
        (void)addr;
        (void)code_offset;
        (void)buffer_data;
        (void)buffer_size;
        return 0;  // No code to copy
    }

    bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept override
    {
        (void)addr;
        (void)beneficiary;
        return false;  // Pretend it failed
    }

    evmc::Result call(const evmc_message& msg) noexcept override
    {
        (void)msg;
        // Return a simple failure result
        evmc_result result{};
        result.status_code = EVMC_REVERT;
        result.gas_left = 0;
        result.gas_refund = 0;
        result.output_data = nullptr;
        result.output_size = 0;
        result.release = nullptr;
        return evmc::Result{result};
    }

    evmc_tx_context get_tx_context() const noexcept override
    {
        evmc_tx_context ctx{};
        // Zero-initialize everything
        std::memset(&ctx, 0, sizeof(ctx));
        ctx.block_gas_limit = 30000000;  // Some reasonable default
        ctx.chain_id = evmc::bytes32{1};  // Ethereum mainnet
        return ctx;
    }

    evmc::bytes32 get_block_hash(int64_t block_number) const noexcept override
    {
        (void)block_number;
        return evmc::bytes32{};  // Return zero hash
    }

    void emit_log(const evmc::address& addr,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc::bytes32 topics[],
                  size_t num_topics) noexcept override
    {
        (void)addr;
        (void)data;
        (void)data_size;
        (void)topics;
        (void)num_topics;
        // Do nothing
    }

    evmc_access_status access_account(const evmc::address& addr) noexcept override
    {
        (void)addr;
        return EVMC_ACCESS_COLD;  // Pretend it's a cold access
    }

    evmc_access_status access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept override
    {
        (void)addr;
        (void)key;
        return EVMC_ACCESS_COLD;  // Pretend it's a cold access
    }

    evmc::bytes32 get_transient_storage(const evmc::address& addr,
                                        const evmc::bytes32& key) const noexcept override
    {
        (void)addr;
        (void)key;
        return evmc::bytes32{};  // Return zero
    }

    void set_transient_storage(const evmc::address& addr,
                               const evmc::bytes32& key,
                               const evmc::bytes32& value) noexcept override
    {
        (void)addr;
        (void)key;
        (void)value;
        // Do nothing
    }
};

} // namespace monad::vm::runtime
