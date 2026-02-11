// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../evmone_host/mpt_hash.hpp"
#include "../evmone_host/requests.hpp"
#include "../evmone_host/rlp.hpp"
#include "../evmone_host/system_contracts.hpp"
#include "blockchaintest.hpp"

namespace evmone::test
{

// Helper functions for blockchain test execution
TransitionResult apply_block(TestState& state, evmc::VM& vm, const state::BlockInfo& block,
    const state::BlockHashes& block_hashes, const std::vector<state::Transaction>& txs,
    evmc_revision rev, std::optional<int64_t> block_reward)
{
    TestState block_state(state);
    system_call_block_start(block_state, block, block_hashes, rev, vm);

    std::vector<state::Log> txs_logs;
    int64_t block_gas_left = block.gas_limit;
    auto blob_gas_left = static_cast<int64_t>(block.blob_gas_used.value_or(0));

    std::vector<RejectedTransaction> rejected_txs;
    std::vector<state::TransactionReceipt> receipts;

    int64_t cumulative_gas_used = 0;

    for (size_t i = 0; i < txs.size(); ++i)
    {
        const auto& tx = txs[i];

        const auto computed_tx_hash = keccak256(rlp::encode(tx));
        auto res = test::transition(
            block_state, block, block_hashes, tx, rev, vm, block_gas_left, blob_gas_left);

        if (holds_alternative<std::error_code>(res))
        {
            const auto ec = std::get<std::error_code>(res);
            rejected_txs.push_back({computed_tx_hash, i, ec.message()});
        }
        else
        {
            auto& receipt = get<state::TransactionReceipt>(res);

            const auto& tx_logs = receipt.logs;

            txs_logs.insert(txs_logs.end(), tx_logs.begin(), tx_logs.end());
            cumulative_gas_used += receipt.gas_used;
            receipt.cumulative_gas_used = cumulative_gas_used;
            if (rev < EVMC_BYZANTIUM)
                receipt.post_state = state::mpt_hash(block_state);

            block_gas_left -= receipt.gas_used;
            blob_gas_left -= static_cast<int64_t>(tx.blob_gas_used());
            receipts.emplace_back(std::move(receipt));
        }
    }

    std::vector<state::Requests> requests;
    if (rev >= EVMC_PRAGUE)
        requests.emplace_back(collect_deposit_requests(receipts));

    auto system_call_requests = system_call_block_end(block_state, block, block_hashes, rev, vm);
    std::ranges::move(system_call_requests, std::back_inserter(requests));

    finalize(block_state, rev, block.coinbase, block_reward, block.ommers, block.withdrawals);

    const auto bloom = compute_bloom_filter(receipts);

    return {std::move(receipts), std::move(rejected_txs), std::move(requests), cumulative_gas_used,
        bloom, blob_gas_left, std::move(block_state)};
}

bool validate_block(
    evmc_revision rev, const TestBlock& test_block, const BlockHeader& parent_header) noexcept
{
    // NOTE: includes only block validity unrelated to individual txs. See `apply_block`.

    if (rev >= EVMC_CANCUN)
    {
        // `excess_blob_gas` and `blob_gas_used` mandatory after Cancun and invalid before.
        if (!test_block.block_info.excess_blob_gas.has_value() ||
            !test_block.block_info.blob_gas_used.has_value())
            return false;

        // Check that the excess blob gas was updated correctly.
        if (*test_block.block_info.excess_blob_gas !=
            state::calc_excess_blob_gas(rev, parent_header.blob_gas_used.value_or(0),
                parent_header.excess_blob_gas.value_or(0)))
            return false;

        // Ensure the total blob gas spent is at most equal to the limit
        if (*test_block.block_info.blob_gas_used > state::max_blob_gas_per_block(rev))
            return false;
    }
    else
    {
        if (test_block.block_info.excess_blob_gas.has_value() ||
            test_block.block_info.blob_gas_used.has_value())
            return false;
    }
    return true;
}

std::optional<int64_t> mining_reward(evmc_revision rev) noexcept
{
    if (rev < EVMC_BYZANTIUM)
        return 5'000000000'000000000;
    if (rev < EVMC_CONSTANTINOPLE)
        return 3'000000000'000000000;
    if (rev < EVMC_PARIS)
        return 2'000000000'000000000;
    return std::nullopt;
}

}  // namespace evmone::test
