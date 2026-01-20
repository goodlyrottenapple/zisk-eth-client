# Monad EVM Client for Zisk

This is a Zisk client that uses the Monad EVM interpreter for executing Ethereum bytecode.

## Overview

This client provides a bridge between the Zisk proving system and the Monad EVM interpreter. It reads EVM bytecode and executes it using Monad's high-performance interpreter implementation.

## Architecture

The client consists of three main components:

1. **C++ Wrapper** (`cpp/wrapper.cpp`): A C-compatible FFI wrapper around the Monad interpreter
2. **Rust FFI Bindings** (`src/ffi.rs`): Safe Rust bindings for calling the C++ code
3. **Main Entrypoint** (`src/main.rs`): The Zisk entrypoint that orchestrates execution

## Current Status

⚠️ **Initial Implementation**: This is a first-pass implementation with a placeholder wrapper. The actual Monad interpreter integration is not yet complete.

### What Works
- Project structure and build system
- C++ compilation via CMake
- Rust FFI bindings
- Basic execution flow

### What's Next
- Integrate the actual Monad interpreter code
- Implement proper EVM execution context
- Handle gas metering and execution results
- Add support for state access and external calls

## Building

Prerequisites:
- Rust toolchain (1.70+)
- CMake (3.15+)
- C++ compiler (GCC 14+ or Clang 19+)

```bash
cd ~/zisk-eth-client/bin/client/monad
cargo build --release
```

## Usage

The client reads EVM bytecode from the input and executes it:

```bash
# The input can be:
# 1. A file path to bytecode
# 2. Raw bytecode bytes
# 3. Empty (will look for bytecode.bin in current directory)

# Example with a bytecode file:
echo "path/to/bytecode.bin" | cargo run
```

## Integration with Monad Interpreter

The Monad interpreter source files are located in `cpp/`:
- `interpreter/` - Core interpreter implementation
- `runtime/` - Runtime support
- `evm/` - EVM-specific traits and utilities
- `core/` - Core types (uint256, etc.)
- `evmc/` - EVMC interface library

To complete the integration, the `cpp/wrapper.cpp` file needs to be updated to:
1. Initialize the Monad interpreter with proper context
2. Parse and analyze the bytecode using Monad's intercode system
3. Execute the bytecode and capture results
4. Return execution status, gas used, and output data

## License

**Important**: The Monad interpreter is licensed under GPL v3. This means any code that links with or uses this client must also comply with GPL v3 requirements.

## Directory Structure

```
monad/
├── src/
│   ├── main.rs          # Zisk entrypoint
│   └── ffi.rs           # Rust FFI bindings
├── cpp/
│   ├── wrapper.cpp      # C++ FFI wrapper
│   ├── interpreter/     # Monad interpreter files
│   ├── runtime/         # Runtime support
│   ├── evm/             # EVM utilities
│   ├── core/            # Core types
│   └── evmc/            # EVMC library
├── build.rs             # Build script
├── CMakeLists.txt       # CMake configuration
└── Cargo.toml           # Rust dependencies
```

## Comparison with RSP Client

This client follows a similar pattern to the RSP client but uses the Monad interpreter instead of RSP's executor:

**RSP**: `main.rs` → `rsp_client_executor` → EVM execution
**Monad**: `main.rs` → `ffi.rs` → `wrapper.cpp` → Monad interpreter → EVM execution

Both clients read input via `ziskos::read_input_slice()` and write output via `ziskos::set_output()`.
