#![no_main]
ziskos::entrypoint!(main);

mod ffi;

use ziskos::{read_input_slice, set_output};

fn main() {
    println!("Monad EVM Client - Starting execution");

    // Read input from ziskos
    let input = read_input_slice();

    if input.is_empty() {
        println!("Error: No input provided");
        return;
    }

    // Deserialize the bytecode from the input using bincode
    let bytecode: Vec<u8> = match bincode::deserialize(&input) {
        Ok(bytecode) => bytecode,
        Err(e) => {
            println!("Failed to deserialize bytecode from input: {}", e);
            return;
        }
    };

    println!("Executing bytecode of length: {} bytes", bytecode.len());

    // Execute the bytecode using the Monad interpreter
    let gas_limit = 10_000_000; // 10 million gas
    match ffi::execute_bytecode(&bytecode, gas_limit) {
        Ok(result) => {
            println!("Execution completed successfully");
            println!("Status: {}", result.status_code);
            println!("Gas left: {}", result.gas_left);
            println!("Gas refund: {}", result.gas_refund);
            println!("Output length: {} bytes", result.output.len());

            if !result.output.is_empty() {
                // Print output data as hex
                println!("Output data (hex): {}", hex::encode(&result.output));

                // Write output to ziskos public output
                // We'll write the output in chunks of 4 bytes as u32 values
                for (index, chunk) in result.output.chunks(4).enumerate() {
                    let mut bytes = [0u8; 4];
                    bytes[..chunk.len()].copy_from_slice(chunk);
                    let value = u32::from_le_bytes(bytes);
                    set_output(index, value);
                }
            }
        }
        Err(e) => {
            println!("Execution failed: {}", e);
        }
    }
}
