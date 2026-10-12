//! The header chain the guest serves BLOCKHASH from.
//!
//! `WitnessBlockHashBuffer` expects the headers ascending and contiguous, the last one
//! being the block's parent, and aborts on any BLOCKHASH outside that range. reth already
//! emits exactly the span from the lowest BLOCKHASH target to the parent, so this only
//! validates; it does not fetch or reorder.

use crate::{Error, Result};
use alloy_consensus::Header;
use alloy_primitives::{keccak256, Bytes, B256};
use alloy_rlp::Decodable;

/// Check ascending numbers, contiguity, the parent-hash links, and that the last header
/// hashes to `parent_hash` (taken from the block being converted).
pub fn validate(headers: &[Bytes], parent_hash: B256) -> Result<()> {
    let mut prev: Option<(u64, B256)> = None;
    for raw in headers {
        let header = decode(raw)?;
        if let Some((number, hash)) = prev {
            if header.number != number + 1 {
                return Err(Error::Headers(format!(
                    "headers not contiguous: {number} followed by {}",
                    header.number
                )));
            }
            if header.parent_hash != hash {
                return Err(Error::Headers(format!(
                    "header {} does not link to header {number}",
                    header.number
                )));
            }
        }
        prev = Some((header.number, keccak256(raw)));
    }
    match prev {
        None => Err(Error::Headers(
            "no headers; the parent header is required".into(),
        )),
        Some((number, hash)) if hash != parent_hash => Err(Error::Headers(format!(
            "last header ({number}) hashes to {hash}, block's parent is {parent_hash}"
        ))),
        Some(_) => Ok(()),
    }
}

/// The block's parent: the last header of a chain that passed [`validate`].
pub fn parent(headers: &[Bytes]) -> Result<Header> {
    let raw = headers
        .last()
        .ok_or_else(|| Error::Headers("no headers".into()))?;
    decode(raw)
}

fn decode(raw: &Bytes) -> Result<Header> {
    Header::decode(&mut &raw[..]).map_err(|e| Error::Headers(format!("header rlp: {e}")))
}
