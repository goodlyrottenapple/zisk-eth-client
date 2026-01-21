// Copyright (C) 2025 - Simplified wrapper for Monad interpreter
// This provides a C FFI interface for Rust to call the Monad EVM interpreter

#include <cstdint>
#include <cstring>
#include <span>
#include "opcodes.hpp"
#include "intercode.hpp"
#include "runtime_types.hpp"
#include "allocator.hpp"
#include "cached_allocator.hpp"
#include "dummy_host.hpp"
#include "execute.hpp"

namespace monad::vm::runtime
{
    CachedAllocatorList EvmStackAllocatorMeta::cache_list;
    CachedAllocatorList EvmMemoryAllocatorMeta::cache_list;

    // Global dummy host instance
    static DummyHost global_dummy_host;
}

// C interface for FFI
extern "C" {

// Result structure that can be passed across FFI boundary
struct EvmExecutionResult {
    // Status code: 0 = success, 1 = revert, 2 = failure
    int32_t status_code;

    // Gas used
    int64_t gas_used;

    // Output data length
    uint32_t output_len;

    // Output data (caller must free with free_output)
    uint8_t* output_data;
};

// Simple memory pool for result allocation (avoids new/delete)
static EvmExecutionResult result_storage;
static uint8_t output_storage[4096]; // Max output size
static bool result_in_use = false;

// Execute EVM bytecode
// Parameters:
//   bytecode: The EVM bytecode to execute
//   bytecode_len: Length of bytecode
//   gas_limit: Gas limit for execution
// Returns: EvmExecutionResult (caller must call free_result when done)
EvmExecutionResult* execute_evm_bytecode(
    const uint8_t* bytecode,
    uint32_t bytecode_len,
    int64_t gas_limit)
{
    using namespace monad::vm::runtime;
    if (result_in_use) {
        return nullptr;
    }

    if (bytecode_len == 0) {
        return nullptr;
    }

    result_in_use = true;

    // Create Intercode wrapper for the bytecode
    std::span<const uint8_t> code_span(bytecode, bytecode_len);
    monad::vm::interpreter::Intercode icode{code_span};

    // Create a minimal evmc_message for Context::from
    evmc_message msg{};
    msg.kind = EVMC_CALL;
    msg.flags = 0;
    msg.depth = 0;
    msg.gas = gas_limit;
    msg.recipient = evmc::address{};
    msg.sender = evmc::address{};
    msg.input_data = nullptr;
    msg.input_size = 0;
    msg.value = evmc::bytes32{};
    msg.create2_salt = evmc::bytes32{};

    EvmMemoryAllocator mem_alloc{};
    auto ctx = Context::from(
        mem_alloc,
        &evmc::Host::get_interface(),
        global_dummy_host.to_context(),
        &msg,
        code_span
    );

    // Allocate stack for execution
    EvmStackAllocator stack_alloc{};
    auto stack_ptr = stack_alloc.allocate();

    // Execute the bytecode using Monad Zero traits
    using traits = monad::MonadTraits<MONAD_ZERO>;
    monad::vm::interpreter::execute<traits>(ctx, icode, stack_ptr.get());

    // Extract result status
    result_storage.status_code = static_cast<int32_t>(ctx.result.status);
    result_storage.gas_used = gas_limit - ctx.gas_remaining;

    // Extract output data from memory if successful or reverted
    using enum StatusCode;
    if (ctx.result.status == Success || ctx.result.status == Revert) {
        // Convert offset and size from 32-byte arrays to uint256_t
        uint256_t offset_val;
        uint256_t size_val;
        std::memcpy(&offset_val, ctx.result.offset, 32);
        std::memcpy(&size_val, ctx.result.size, 32);

        // Get the actual numeric values (assuming they fit in size_t)
        // For EVM, we need to check if the values are within reasonable bounds
        if (offset_val <= SIZE_MAX && size_val <= 4096) {
            size_t offset = static_cast<size_t>(offset_val);
            size_t size = static_cast<size_t>(size_val);

            // Copy data from memory to output storage
            if (size > 0 && offset + size <= ctx.memory.size) {
                std::memcpy(output_storage, ctx.memory.data + offset, size);
                result_storage.output_len = static_cast<uint32_t>(size);
                result_storage.output_data = output_storage;
            } else {
                result_storage.output_len = 0;
                result_storage.output_data = nullptr;
            }
        } else {
            result_storage.output_len = 0;
            result_storage.output_data = nullptr;
        }
    } else {
        result_storage.output_len = 0;
        result_storage.output_data = nullptr;
    }

    return &result_storage;
}

// Free the result structure
void free_result(EvmExecutionResult* result) {
    if (result == &result_storage) {
        result_in_use = false;
    }
}

} // extern "C"
