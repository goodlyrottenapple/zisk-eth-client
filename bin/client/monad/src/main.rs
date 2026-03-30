#![no_main]
ziskos::entrypoint!(main);

mod ffi;

mod zkvm_accelerators;

/// Read input: on x86 from CLI arg (`-i <file>`), on RISC-V from ziskos memory-mapped input.
fn read_input() -> Vec<u8> {
    #[cfg(all(target_os = "zkvm", target_vendor = "zisk"))]
    {
        ziskos::io::read_input_slice().to_vec()
    }

    #[cfg(not(all(target_os = "zkvm", target_vendor = "zisk")))]
    {
        let args: Vec<String> = std::env::args().collect();
        let path = match args.iter().position(|a| a == "-i") {
            Some(i) if i + 1 < args.len() => args[i + 1].clone(),
            _ => {
                eprintln!("Usage: {} -i <input.bin>", args.first().map_or("zec-monad", |s| s.as_str()));
                std::process::exit(1);
            }
        };
        let bytes = std::fs::read(&path).unwrap_or_else(|e| {
            eprintln!("Error reading {}: {}", path, e);
            std::process::exit(1);
        });
        // Skip the 8-byte length prefix added for the ZisK emulator
        // (ziskos::io::read_input_slice strips it automatically on zkvm)
        bytes.get(8..).unwrap_or(&[]).to_vec()
    }
}

fn main() {
    println!("Monad EVM Client - Blockchain Test Execution");

    let input = read_input();

    if input.is_empty() {
        println!("Error: No input provided");
        return;
    }

    println!("Test data size: {} bytes", input.len());

    // Execute blockchain test (input is binary format produced by ethtest_converter)
    let passed = ffi::execute_blockchain_test_safe(&input);

    if passed {
        println!("✓ Test passed");
        ziskos::io::write(&[1u8]);
    } else {
        println!("✗ Test failed");
        ziskos::io::write(&[0u8]);
    }
}
