// FFI bindings to the Monad C++ interpreter wrapper

use std::slice;

#[repr(C)]
pub struct EvmExecutionResult {
    pub status_code: i32,
    pub gas_used: i64,
    pub output_len: u32,
    pub output_data: *mut u8,
}

extern "C" {
    pub fn execute_evm_bytecode(
        bytecode: *const u8,
        bytecode_len: u32,
        gas_limit: i64,
    ) -> *mut EvmExecutionResult;

    pub fn free_result(result: *mut EvmExecutionResult);
}

pub struct ExecutionResult {
    pub status_code: i32,
    pub gas_used: i64,
    pub output: Vec<u8>,
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

        let output = if result.output_len > 0 && !result.output_data.is_null() {
            slice::from_raw_parts(result.output_data, result.output_len as usize).to_vec()
        } else {
            Vec::new()
        };

        let execution_result = ExecutionResult {
            status_code: result.status_code,
            gas_used: result.gas_used,
            output,
        };

        free_result(result_ptr);

        Ok(execution_result)
    }
}
