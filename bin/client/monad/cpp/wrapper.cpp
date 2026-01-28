// Copyright (C) 2025 - Simplified wrapper for Monad interpreter
// This provides a C FFI interface for Rust to call the Monad EVM interpreter

#include <cstdint>
#include <cstring>
#include <span>
#include "evm/opcodes.hpp"
#include "runtime/types.hpp"
#include "runtime/allocator.hpp"
#include "runtime/cached_allocator.hpp"
#include "evmone_host/host.hpp"
#include "evmone_host/test_state.hpp"
#include "evmone_host/test_vm.hpp"
#include "evmone_host/block.hpp"
#include "evmone_host/transaction.hpp"

namespace monad::vm::runtime
{
    CachedAllocatorList EvmStackAllocatorMeta::cache_list;
    CachedAllocatorList EvmMemoryAllocatorMeta::cache_list;
}

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
    if (global_vm == nullptr) {
        init_evm_vm();
    }

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

} // extern "C"
