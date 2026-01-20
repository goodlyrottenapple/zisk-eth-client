// Copyright (C) 2025 - Simplified wrapper for Monad interpreter
// This provides a C FFI interface for Rust to call the Monad EVM interpreter

#include <cstdint>
#include <cstring>
#include <span>
#include "opcodes.hpp"
#include "intercode.hpp"

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
static monad::vm::interpreter::Intercode* current_intercode = nullptr;

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
    if (result_in_use) {
        return nullptr;
    }

    if (bytecode_len == 0) {
        return nullptr;
    }

    result_in_use = true;

    // Clean up any previous Intercode
    if (current_intercode) {
        delete current_intercode;
        current_intercode = nullptr;
    }

    // Create Intercode wrapper for the bytecode
    std::span<const uint8_t> code_span(bytecode, bytecode_len);
    current_intercode = new monad::vm::interpreter::Intercode(code_span);

    // Read the first byte of the wrapped program
    uint8_t first_opcode = current_intercode->code()[0];

    // Get the opcode info using the opcode table
    // Using Monad Zero traits for now
    using traits = monad::MonadTraits<MONAD_ZERO>;
    const auto& opcode_info = monad::vm::compiler::opcode_table<traits>[first_opcode];

    // Copy the opcode name to output_storage as null-terminated string
    size_t name_len = opcode_info.name.length();
    if (name_len > sizeof(output_storage) - 1) {
        name_len = sizeof(output_storage) - 1;
    }
    memcpy(output_storage, opcode_info.name.data(), name_len);
    output_storage[name_len] = '\0';

    result_storage.status_code = 0; // success
    result_storage.gas_used = 0;
    result_storage.output_len = name_len + 1; // include null terminator
    result_storage.output_data = output_storage;

    return &result_storage;
}

// Free the result structure
void free_result(EvmExecutionResult* result) {
    if (result == &result_storage) {
        result_in_use = false;
        // Clean up Intercode when result is freed
        if (current_intercode) {
            delete current_intercode;
            current_intercode = nullptr;
        }
    }
}

} // extern "C"
