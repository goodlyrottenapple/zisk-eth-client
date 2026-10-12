//! reth `debug_executionWitness` → Monad zkVM witness.
//!
//! The Monad guest executes a block from a self-contained witness: the block RLP, a
//! pre-resolved offset-linked trie of the pre-state nodes the block touches, the contract
//! codes it runs, and the headers BLOCKHASH may ask for. reth's witness carries the same
//! information as a flat, hash-keyed node list. Conversion is structural: no state is
//! fetched, nothing is executed, and whatever reth did not reveal becomes a `Digest` node
//! that the guest refuses to look through.

pub mod convert;
pub mod envelope;
pub mod error;
pub mod headers;
pub mod offset_trie;

pub use alloy_rpc_types_debug::ExecutionWitness;
pub use convert::convert;
pub use error::{Error, Result};
