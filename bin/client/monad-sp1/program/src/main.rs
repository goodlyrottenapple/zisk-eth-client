#![no_main]
extern crate alloc;
sp1_zkvm::entrypoint!(main);

mod ffi;

/// Expose Rust's global allocator to C++ code via FFI.
/// Called by malloc() in syscalls_stubs.cpp.
#[no_mangle]
pub extern "C" fn sp1_alloc_aligned(size: usize, align: usize) -> *mut u8 {
    unsafe {
        let layout = core::alloc::Layout::from_size_align_unchecked(size, align);
        alloc::alloc::alloc(layout)
    }
}

fn main() {
    let input: Vec<u8> = sp1_zkvm::io::read_vec();

    if input.is_empty() {
        sp1_zkvm::io::commit(&0u32);
        return;
    }

    let passed = ffi::execute_blockchain_test_safe(&input);

    if passed {
        sp1_zkvm::io::commit(&1u32);
    } else {
        sp1_zkvm::io::commit(&0u32);
    }
}
