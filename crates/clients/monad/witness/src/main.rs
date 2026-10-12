//! Offline conversion: a saved `debug_executionWitness` result plus the raw block.
//!
//! ```text
//! monad-witness --witness 25815142.json --block 25815142.rlp --out 25815142.witness
//! ```
//!
//! The live path (fetch from a reth node) is `input-monad`'s `MonadClient::from_rpc`.

use anyhow::{Context, Result};
use clap::Parser;
use monad_witness::ExecutionWitness;
use serde::Deserialize;
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(version, about)]
struct Args {
    /// reth `debug_executionWitness` result (bare object or JSON-RPC response).
    #[arg(long)]
    witness: PathBuf,
    /// The block's raw RLP.
    #[arg(long)]
    block: PathBuf,
    /// Output `.witness` path.
    #[arg(long)]
    out: PathBuf,
}

/// A JSON-RPC response envelope, so a raw `curl` capture is accepted as-is.
#[derive(Deserialize)]
struct RpcResponse {
    result: ExecutionWitness,
}

fn main() -> Result<()> {
    let args = Args::parse();
    let json = std::fs::read(&args.witness)
        .with_context(|| format!("reading {}", args.witness.display()))?;
    let witness = match serde_json::from_slice::<ExecutionWitness>(&json) {
        Ok(bare) => bare,
        Err(_) => {
            serde_json::from_slice::<RpcResponse>(&json)
                .context("debug_executionWitness JSON")?
                .result
        }
    };
    let block_rlp =
        std::fs::read(&args.block).with_context(|| format!("reading {}", args.block.display()))?;
    let out = monad_witness::convert(&witness, &block_rlp)?;
    std::fs::write(&args.out, &out).with_context(|| format!("writing {}", args.out.display()))?;
    eprintln!(
        "{}: {} bytes ({} nodes, {} codes, {} headers)",
        args.out.display(),
        out.len(),
        witness.state.len(),
        witness.codes.len(),
        witness.headers.len()
    );
    Ok(())
}
