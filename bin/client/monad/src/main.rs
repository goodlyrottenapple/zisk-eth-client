#![no_main]
ziskos::entrypoint!(main);

mod ffi;

use ziskos::{read_input_slice, set_output};

fn main() {
    println!("Monad EVM Client - Blockchain Test Execution");

    // Read binary test data from ziskos input
    let input = read_input_slice();

    if input.is_empty() {
        println!("Error: No input provided");
        return;
    }

    println!("Test data size: {} bytes", input.len());

    // Execute blockchain test (input is binary format produced by ethtest_converter)
    let passed = ffi::execute_blockchain_test_safe(&input);

    if passed {
        println!("✓ Test passed");
        set_output(0, 1);
    } else {
        println!("✗ Test failed");
        set_output(0, 0);
    }
}
