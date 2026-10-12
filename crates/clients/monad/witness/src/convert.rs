//! Top-level pipeline.

use crate::{envelope, headers, offset_trie::OffsetTrie, Error, ExecutionWitness, Result};
use alloy_primitives::B256;
use alloy_rlp::Decodable;

/// Convert a reth witness and the block's raw RLP into the bytes of a Monad `.witness`.
/// `witness.keys` (address and slot preimages) are not needed: Monad leaves carry the
/// remaining path nibbles, which the leaf RLP already holds.
pub fn convert(witness: &ExecutionWitness, block_rlp: &[u8]) -> Result<Vec<u8>> {
    let parent_hash = parent_hash_of(block_rlp)?;
    headers::validate(&witness.headers, parent_hash)?;
    let state_root = headers::parent(&witness.headers)?.state_root;
    let trie = OffsetTrie::write(&witness.state, state_root)?;
    Ok(envelope::encode(
        block_rlp,
        &trie,
        &witness.codes,
        &witness.headers,
    ))
}

/// The `parentHash` field of a raw block (first field of its header).
fn parent_hash_of(block_rlp: &[u8]) -> Result<B256> {
    let bad = |e: alloy_rlp::Error| Error::Parse(format!("block rlp: {e}"));
    let mut buf = block_rlp;
    let outer = alloy_rlp::Header::decode(&mut buf).map_err(bad)?;
    if !outer.list {
        return Err(Error::Parse("block rlp is not a list".into()));
    }
    let header = alloy_consensus::Header::decode(&mut buf).map_err(bad)?;
    Ok(header.parent_hash)
}
