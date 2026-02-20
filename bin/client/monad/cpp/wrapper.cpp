// Copyright (C) 2025 - Simplified wrapper for Monad interpreter
// This provides a C FFI interface for Rust to call the Monad EVM interpreter

#include <cstdint>
#include <cstring>
#include <span>
#include "evmone_host/host.hpp"
#include "evmone_host/test_state.hpp"
#include "evmone_host/test_vm.hpp"
#include "evmone_host/block.hpp"
#include "evmone_host/transaction.hpp"
#include "evmone_host/mpt_hash.hpp"
#include "evmone_test/blockchaintest.hpp"
#include "converter/binary_format.hpp"

// C interface for FFI
extern "C" {

// Simple memory pool for result allocation (avoids new/delete)
static evmc_result result_storage;
static evmone::test::TestVM* global_vm = nullptr;

// Initialize the global VM
void init_evm_vm() {
    if (global_vm == nullptr) {
        global_vm = new evmone::test::TestVM();
    }
}

// Execute EVM bytecode
// Parameters:
//   bytecode: The EVM bytecode to execute
//   bytecode_len: Length of bytecode
//   gas_limit: Gas limit for execution
// Returns: evmc_result (caller must call free_result when done)
evmc_result* execute_evm_bytecode(
    const uint8_t* bytecode,
    uint32_t bytecode_len,
    int64_t gas_limit)
{
    // Initialize VM if needed
    init_evm_vm();

    // Create test state
    evmone::test::TestState state;

    // Create block info with default values
    evmone::state::BlockInfo block;
    block.number = 1;
    block.timestamp = 0;
    block.gas_limit = 30000000;
    block.coinbase = evmc::address{};
    block.difficulty = 0;
    block.base_fee = 0;

    // Create block hashes
    evmone::test::TestBlockHashes block_hashes;

    // Create transaction
    evmone::state::Transaction tx;
    tx.sender = evmc::address{};
    tx.to = evmc::address{};
    tx.gas_limit = gas_limit;
    tx.max_gas_price = 0;
    tx.value = 0;

    // Create evmc_message
    evmc_message msg{};
    msg.kind = EVMC_CALL;
    msg.flags = 0;
    msg.depth = 0;
    msg.gas = gas_limit;
    msg.recipient = evmc::address{};
    msg.sender = evmc::address{};
    msg.input_data = nullptr;
    msg.input_size = 0;
    msg.value = evmc::uint256be{};
    msg.create2_salt = evmc::bytes32{};
    msg.code_address = evmc::address{};

    // Create host
    evmone::state::State s(state);
    evmc::VM vm_wrapper{static_cast<evmc_vm*>(global_vm)};
    evmone::state::Host host(EVMC_CANCUN, vm_wrapper, s, block, block_hashes, tx);

    // Execute via EVMC interface
    result_storage = global_vm->execute(
        global_vm,
        &host.get_interface(),
        host.to_context(),
        EVMC_CANCUN,
        &msg,
        bytecode,
        bytecode_len
    );

    return &result_storage;
}

// Free the result structure
void free_result(evmc_result* result) {
    // Call the release function if present
    if (result->release) {
        result->release(result);
    }
}

// Execute a blockchain test
// Parameters:
//   test_data: Binary test data (produced by ethtest_converter)
//   test_data_len: Length of test data
// Returns: true if test passes, false if test fails
bool execute_blockchain_test(
    const uint8_t* test_data,
    uint32_t test_data_len)
{
    printf("execute_blockchain_test: Starting\n");

    // Initialize VM if needed
    init_evm_vm();
    printf("VM initialized\n");

    // Deserialize binary format
    auto tests = binary_format::deserialize_blockchain_tests(test_data, test_data_len);
    printf("Tests loaded: %zu tests\n", tests.size());

    if (tests.empty()) {
        printf("ERROR: No tests found\n");
        return false;  // No tests found
    }

    // Execute the first test (currently only supporting single test)
    const auto& test = tests[0];
    printf("Test name: %s\n", test.name.c_str());
    printf("Pre-state accounts: %zu\n", test.pre_state.size());
    printf("Test blocks: %zu\n", test.test_blocks.size());

        // Validate genesis block header
        printf("Validating genesis block header\n");
        if (test.genesis_block_header.block_number != 0) {
            printf("ERROR: Genesis block number != 0: %ld\n", test.genesis_block_header.block_number);
            return false;
        }
        if (test.genesis_block_header.gas_used != 0) {
            printf("ERROR: Genesis gas used != 0: %ld\n", test.genesis_block_header.gas_used);
            return false;
        }
        if (test.genesis_block_header.transactions_root != evmone::state::EMPTY_MPT_HASH) {
            printf("ERROR: Genesis transactions root mismatch\n");
            return false;
        }
        if (test.genesis_block_header.receipts_root != evmone::state::EMPTY_MPT_HASH) {
            printf("ERROR: Genesis receipts root mismatch\n");
            return false;
        }
        printf("Genesis validation passed\n");

        // Initialize state
        auto state = test.pre_state;

        // Initialize block hashes
        evmone::test::TestBlockHashes block_hashes{
            {test.genesis_block_header.block_number, test.genesis_block_header.hash}
        };

        // Wrap the global VM
        evmc::VM vm_wrapper{static_cast<evmc_vm*>(global_vm)};

        // Execute each test block
        printf("Executing %zu test blocks\n", test.test_blocks.size());
        for (size_t i = 0; i < test.test_blocks.size(); ++i) {
            printf("\nExecuting block %zu\n", i);
            const auto& test_block = test.test_blocks[i];
            const auto& parent_header =
                i == 0 ? test.genesis_block_header : test.test_blocks[i - 1].expected_block_header;

            const auto rev = test.rev.get_revision(test_block.block_info.timestamp);
            printf("Block revision: %d\n", rev);
            printf("Block valid flag: %d\n", test_block.valid);
            printf("Block transactions: %zu\n", test_block.transactions.size());

            // Validate block structure
            printf("Validating block structure\n");
            if (!evmone::test::validate_block(rev, test_block, parent_header)) {
                printf("Block validation failed\n");
                if (test_block.valid) {
                    printf("ERROR: Expected valid block failed validation\n");
                    return false;  // Expected valid block failed validation
                }
                printf("Invalid block correctly rejected\n");
                continue;  // Invalid block correctly rejected
            }
            printf("Block validation passed\n");

            // Execute block
            printf("Applying block\n");
            auto result = evmone::test::apply_block(
                state, vm_wrapper, test_block.block_info, block_hashes,
                test_block.transactions, rev, evmone::test::mining_reward(rev)
            );
            printf("Block applied, gas used: %ld\n", result.gas_used);

            // Log storage deltas
            printf("\n=== Storage Deltas ===\n");
            for (const auto& [addr, new_account] : result.block_state) {
                auto old_it = state.find(addr);

                // Check if account existed before
                bool account_existed = (old_it != state.end());

                if (!account_existed) {
                    // New account created - print if it has any meaningful state
                    bool has_state = new_account.balance != 0 || new_account.nonce != 0 ||
                                    !new_account.code.empty() || !new_account.storage.empty();

                    if (has_state) {
                        printf("Account %s (NEW):\n", evmc::hex(addr).c_str());
                        printf("  Balance: %s\n", intx::to_string(new_account.balance, 16).c_str());
                        printf("  Nonce: %lu\n", new_account.nonce);
                        printf("  Code size: %zu bytes\n", new_account.code.size());

                        if (!new_account.storage.empty()) {
                            printf("  Storage entries: %zu\n", new_account.storage.size());
                            for (const auto& [key, value] : new_account.storage) {
                                printf("    [%s] = %s\n",
                                       evmc::hex(key).c_str(),
                                       evmc::hex(value).c_str());
                            }
                        }
                    }
                } else {
                    const auto& old_account = old_it->second;

                    // Check what changed
                    bool balance_changed = (old_account.balance != new_account.balance);
                    bool nonce_changed = (old_account.nonce != new_account.nonce);
                    bool code_changed = (old_account.code != new_account.code);

                    // Check for storage changes
                    bool storage_changed = false;
                    std::vector<std::tuple<evmc::bytes32, evmc::bytes32, evmc::bytes32>> storage_changes;

                    for (const auto& [key, new_value] : new_account.storage) {
                        auto old_value_it = old_account.storage.find(key);
                        auto old_value = (old_value_it != old_account.storage.end()) ?
                                         old_value_it->second : evmc::bytes32{};

                        if (old_value != new_value) {
                            storage_changes.push_back({key, old_value, new_value});
                            storage_changed = true;
                        }
                    }

                    // Check for deleted storage slots
                    std::vector<std::pair<evmc::bytes32, evmc::bytes32>> deleted_storage;
                    for (const auto& [key, old_value] : old_account.storage) {
                        if (new_account.storage.find(key) == new_account.storage.end()) {
                            deleted_storage.push_back({key, old_value});
                            storage_changed = true;
                        }
                    }

                    // Print if anything changed
                    if (balance_changed || nonce_changed || code_changed || storage_changed) {
                        printf("Account %s:\n", evmc::hex(addr).c_str());

                        if (balance_changed) {
                            printf("  Balance: %s -> %s\n",
                                   intx::to_string(old_account.balance, 16).c_str(),
                                   intx::to_string(new_account.balance, 16).c_str());
                        }

                        if (nonce_changed) {
                            printf("  Nonce: %lu -> %lu\n", old_account.nonce, new_account.nonce);
                        }

                        if (code_changed) {
                            printf("  Code size: %zu -> %zu bytes\n",
                                   old_account.code.size(), new_account.code.size());
                        }

                        for (const auto& [key, old_value, new_value] : storage_changes) {
                            printf("  Storage [%s]: %s -> %s\n",
                                   evmc::hex(key).c_str(),
                                   evmc::hex(old_value).c_str(),
                                   evmc::hex(new_value).c_str());
                        }

                        for (const auto& [key, old_value] : deleted_storage) {
                            printf("  Storage [%s]: %s -> DELETED\n",
                                   evmc::hex(key).c_str(),
                                   evmc::hex(old_value).c_str());
                        }
                    }
                }
            }

            // Check for deleted accounts
            for (const auto& [addr, old_account] : state) {
                if (result.block_state.find(addr) == result.block_state.end()) {
                    printf("Account %s: DELETED\n", evmc::hex(addr).c_str());
                }
            }
            printf("=== End Storage Deltas ===\n\n");

            // For valid blocks, verify results
            if (test_block.valid) {
                printf("Verifying valid block results\n");

                // Check for rejected transactions
                if (!result.rejected.empty()) {
                    printf("ERROR: %zu transactions rejected\n", result.rejected.size());
                    return false;
                }

                // Check blob gas usage
                if (result.blob_gas_left != 0) {
                    printf("ERROR: Blob gas left != 0: %ld\n", result.blob_gas_left);
                    return false;
                }

                // Verify state root
                auto computed_state_root = evmone::state::mpt_hash(result.block_state);
                if (computed_state_root != test_block.expected_block_header.state_root) {
                    printf("ERROR: State root mismatch\n");
                    printf("  Expected: %s\n", evmc::hex(test_block.expected_block_header.state_root).c_str());
                    printf("  Computed: %s\n", evmc::hex(computed_state_root).c_str());
                    return false;
                }

                // Verify receipts root
                auto computed_receipts_root = evmone::state::mpt_hash(result.receipts);
                if (computed_receipts_root != test_block.expected_block_header.receipts_root) {
                    printf("ERROR: Receipts root mismatch\n");
                    printf("  Expected: %s\n", evmc::hex(test_block.expected_block_header.receipts_root).c_str());
                    printf("  Computed: %s\n", evmc::hex(computed_receipts_root).c_str());
                    return false;
                }

                // Verify gas used
                if (result.gas_used != test_block.expected_block_header.gas_used) {
                    printf("ERROR: Gas used mismatch: expected %ld, got %ld\n",
                           test_block.expected_block_header.gas_used, result.gas_used);
                    return false;
                }

                printf("Block verification passed\n");

                // Update state and block hashes for next block
                state = result.block_state;
                block_hashes[test_block.expected_block_header.block_number] =
                    test_block.expected_block_header.hash;
            } else {
                // Block expected to be invalid but passed validation
                return false;
            }
        }

        // Verify final state hash
        printf("\nVerifying final state hash\n");
        const auto expected_post_hash =
            std::holds_alternative<evmone::test::TestState>(test.expectation.post_state) ?
                evmone::state::mpt_hash(std::get<evmone::test::TestState>(test.expectation.post_state)) :
                std::get<evmone::hash256>(test.expectation.post_state);

        auto final_state_hash = evmone::state::mpt_hash(state);
        if (final_state_hash != expected_post_hash) {
            printf("ERROR: Final state hash mismatch\n");
            printf("  Expected: %s\n", evmc::hex(expected_post_hash).c_str());
            printf("  Computed: %s\n", evmc::hex(final_state_hash).c_str());
            return false;
        }

        printf("Final state hash verification passed\n");
        printf("✓ All checks passed\n");
        return true;  // All checks passed
}

} // extern "C"
