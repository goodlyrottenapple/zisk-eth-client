# input-gen-bytecode

A command-line tool to generate binary input files from EVM bytecode hex strings.

## Usage

```bash
input-gen-bytecode <BYTECODE> --output <FILE>
```

### Arguments

- `<BYTECODE>`: EVM bytecode as a hex string (with or without `0x` prefix)
- `-o, --output <FILE>`: Output file path

### Examples

With `0x` prefix:
```bash
input-gen-bytecode 0x6080604052 -o bytecode.bin
```

Without `0x` prefix:
```bash
input-gen-bytecode 6080604052 -o bytecode.bin
```

## Output Format

The output file contains the bytecode serialized using `bincode::serialize()`, which can be deserialized using `bincode::deserialize::<Vec<u8>>()`.

## Building

```bash
cargo build --release
```

The binary will be available at `../../target/release/input-gen-bytecode`.
