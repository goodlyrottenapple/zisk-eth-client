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

// Simple memory pool for result allocation (avoids new/delete)
static evmc_result result_storage;

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
    using namespace monad::vm::runtime;

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

    // Use the context's copy_to_evmc_result method to get the result
    evmc::Result evmc_res = ctx.copy_to_evmc_result();

    // Copy the result to our static storage and release ownership
    result_storage = evmc_res.release_raw();

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
