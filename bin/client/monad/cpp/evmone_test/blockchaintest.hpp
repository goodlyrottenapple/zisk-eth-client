// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../evmone_host/block.hpp"
#include "../evmone_host/bloom_filter.hpp"
#include "../evmone_host/test_state.hpp"
#include "../evmone_host/transaction.hpp"
#include "../evmone_host/requests.hpp"
#include "utils.hpp"
#include <evmc/evmc.hpp>
#include <span>
#include <vector>

namespace evmone::test
{
struct UnsupportedTestFeature : std::runtime_error
{
    using runtime_error::runtime_error;
};

// https://ethereum.org/en/developers/docs/blocks/
struct BlockHeader
{
    hash256 parent_hash;
    address coinbase;
    hash256 state_root;
    hash256 receipts_root;
    state::BloomFilter logs_bloom;
    int64_t difficulty;
    bytes32 prev_randao;
    int64_t block_number;
    int64_t gas_limit;
    int64_t gas_used;
    int64_t timestamp;
    bytes extra_data;
    uint64_t base_fee_per_gas;
    hash256 hash;
    hash256 transactions_root;
    hash256 withdrawal_root;
    hash256 parent_beacon_block_root;
    std::optional<uint64_t> blob_gas_used;
    std::optional<uint64_t> excess_blob_gas;
    hash256 requests_hash;
};

struct TestBlock
{
    state::BlockInfo block_info;
    std::vector<state::Transaction> transactions;
    bool valid = true;

    BlockHeader expected_block_header;
};

struct BlockchainTest
{
    struct Expectation
    {
        hash256 last_block_hash;
        std::variant<TestState, hash256> post_state;
    };

    std::string name;

    std::vector<TestBlock> test_blocks;
    BlockHeader genesis_block_header;
    TestState pre_state;
    RevisionSchedule rev;

    Expectation expectation;
};

struct RejectedTransaction
{
    hash256 hash;
    size_t index;
    std::string message;
};

struct TransitionResult
{
    std::vector<state::TransactionReceipt> receipts;
    std::vector<RejectedTransaction> rejected;
    std::vector<state::Requests> requests;
    int64_t gas_used;
    state::BloomFilter bloom;
    int64_t blob_gas_left;
    TestState block_state;
};

// Test loading
std::vector<BlockchainTest> load_blockchain_tests(std::istream& input);

// Test execution helpers
TransitionResult apply_block(TestState& state, evmc::VM& vm, const state::BlockInfo& block,
    const state::BlockHashes& block_hashes, const std::vector<state::Transaction>& txs,
    evmc_revision rev, std::optional<int64_t> block_reward);

bool validate_block(evmc_revision rev, const TestBlock& test_block,
    const BlockHeader& parent_header) noexcept;

std::optional<int64_t> mining_reward(evmc_revision rev) noexcept;

// Legacy test runner (not used in wrapper)
void run_blockchain_tests(std::span<const BlockchainTest> tests, evmc::VM& vm);
void blockchain_test_setup(int* argc, char** argv);
int blockchain_test_main(std::vector<std::string> const& paths, bool trace_flag, evmc::VM& vm);

}  // namespace evmone::test
