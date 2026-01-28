// FFI bindings to the Monad C++ interpreter wrapper

use std::slice;

// EVMC types matching the C struct definitions
#[repr(C)]
pub struct EvmcAddress {
    pub bytes: [u8; 20],
}

#[repr(C)]
pub struct EvmcResult {
    pub status_code: i32,
    pub gas_left: i64,
    pub gas_refund: i64,
    pub output_data: *const u8,
    pub output_size: usize,
    pub release: Option<unsafe extern "C" fn(*const EvmcResult)>,
    pub create_address: EvmcAddress,
    pub padding: [u8; 4],
}

extern "C" {
    pub fn execute_evm_bytecode(
        bytecode: *const u8,
        bytecode_len: u32,
        gas_limit: i64,
    ) -> *mut EvmcResult;

    pub fn free_result(result: *mut EvmcResult);
}

pub struct ExecutionResult {
    pub status_code: i32,
    pub gas_left: i64,
    pub gas_refund: i64,
    pub output: Vec<u8>,
}

impl ExecutionResult {
    pub fn status_str(&self) -> &'static str {
        match self.status_code {
            0 => "EVMC_SUCCESS",
            1 => "EVMC_FAILURE",
            2 => "EVMC_REVERT",
            3 => "EVMC_OUT_OF_GAS",
            4 => "EVMC_INVALID_INSTRUCTION",
            5 => "EVMC_UNDEFINED_INSTRUCTION",
            6 => "EVMC_STACK_OVERFLOW",
            7 => "EVMC_STACK_UNDERFLOW",
            8 => "EVMC_BAD_JUMP_DESTINATION",
            9 => "EVMC_INVALID_MEMORY_ACCESS",
            10 => "EVMC_CALL_DEPTH_EXCEEDED",
            11 => "EVMC_STATIC_MODE_VIOLATION",
            12 => "EVMC_PRECOMPILE_FAILURE",
            13 => "EVMC_CONTRACT_VALIDATION_FAILURE",
            14 => "EVMC_ARGUMENT_OUT_OF_RANGE",
            15 => "EVMC_WASM_UNREACHABLE_INSTRUCTION",
            16 => "EVMC_WASM_TRAP",
            17 => "EVMC_INSUFFICIENT_BALANCE",
            -1 => "EVMC_INTERNAL_ERROR",
            -2 => "EVMC_REJECTED",
            -3 => "EVMC_OUT_OF_MEMORY",
            _ => "UNKNOWN_STATUS",
        }
    }
}

/// Safe wrapper around the C FFI
pub fn execute_bytecode(bytecode: &[u8], gas_limit: i64) -> Result<ExecutionResult, String> {
    unsafe {
        let result_ptr = execute_evm_bytecode(
            bytecode.as_ptr(),
            bytecode.len() as u32,
            gas_limit,
        );

        if result_ptr.is_null() {
            return Err("Failed to execute bytecode".to_string());
        }

        let result = &*result_ptr;

        let output = if result.output_size > 0 && !result.output_data.is_null() {
            slice::from_raw_parts(result.output_data, result.output_size).to_vec()
        } else {
            Vec::new()
        };

        let execution_result = ExecutionResult {
            status_code: result.status_code,
            gas_left: result.gas_left,
            gas_refund: result.gas_refund,
            output,
        };

        free_result(result_ptr);

        Ok(execution_result)
    }
}
