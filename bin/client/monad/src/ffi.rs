// FFI bindings to the Monad C++ interpreter wrapper

extern "C" {
    pub fn execute_blockchain_test(
        test_data: *const u8,
        test_data_len: u32,
    ) -> bool;
}

/// Safe wrapper for blockchain test execution
pub fn execute_blockchain_test_safe(test_data: &[u8]) -> bool {
    unsafe {
        execute_blockchain_test(test_data.as_ptr(), test_data.len() as u32)
    }
}
