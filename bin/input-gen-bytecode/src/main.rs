use anyhow::{anyhow, Result};
use clap::Parser;
use std::{fs::File, io::Write, path::PathBuf};

#[derive(Debug, Clone, Parser)]
#[clap(
    name = "input-gen-bytecode",
    about = "Generate binary input files from EVM bytecode hex strings"
)]
pub struct Args {
    /// EVM bytecode as a hex string (with or without 0x prefix)
    #[clap(value_name = "BYTECODE")]
    pub bytecode: String,

    /// Output file path
    #[clap(short, long, value_name = "FILE")]
    pub output: PathBuf,
}

fn main() -> Result<()> {
    let args = Args::parse();

    // Remove 0x prefix if present
    let hex_str = args.bytecode.strip_prefix("0x").unwrap_or(&args.bytecode);

    // Convert hex string to Vec<u8>
    let bytecode: Vec<u8> = hex::decode(hex_str)
        .map_err(|e| anyhow!("Failed to decode hex string: {}", e))?;

    if bytecode.is_empty() {
        return Err(anyhow!("Bytecode cannot be empty"));
    }

    // Serialize the bytecode using bincode
    let serialized = bincode::serialize(&bytecode)
        .map_err(|e| anyhow!("Failed to serialize bytecode: {}", e))?;

    // Create parent directory if it doesn't exist
    if let Some(parent) = args.output.parent() {
        if !parent.exists() {
            std::fs::create_dir_all(parent)?;
        }
    }

    // Write to output file
    let mut file = File::create(&args.output)?;
    file.write_all(&serialized)?;

    println!(
        "Successfully wrote {} bytes of bytecode ({} bytes serialized) to {}",
        bytecode.len(),
        serialized.len(),
        args.output.display()
    );

    Ok(())
}
