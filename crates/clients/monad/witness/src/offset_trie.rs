//! The `MZW\x01` offset blob (monad repo, `category/execution/ethereum/db/offset_trie.hpp`),
//! written straight from the witness's flat node list.
//!
//! Layout, as the guest reads it. All integers little-endian, nothing aligned, offsets
//! absolute from byte 0 of the blob, `0` meaning "no node".
//!
//! ```text
//! header        'M' 'Z' 'W' 0x01 | u32 root_offset
//! BRANCH        0x00 | u32 child[16]
//! EXT           0x01 | u32 child | path
//! LEAF_ACCT     0x02 | u32 storage | 0xa0 code_hash[32] | u8 n | rlp(nonce) rlp(balance) (n bytes) | path
//! LEAF_STORAGE  0x03 | value[32] big-endian | path
//! DIGEST        0xa0 | hash[32]
//! path          u8 nibble_count | packed nibbles, first nibble in the high half
//! ```
//!
//! The walk starts at the parent header's state root and follows branch children and
//! extension targets through the node list, indexed by `keccak256(rlp)`. A referenced hash
//! with no preimage becomes a DIGEST: the guest can hash through it but aborts if execution
//! needs to look inside it. A child shorter than 32 bytes is embedded in the parent's RLP and
//! is decoded in place. Account leaves open a second tree: the empty storage root yields no
//! subtree (`storage = 0`), an absent root a single DIGEST, anything else the storage tree.
//! A node several accounts share is in the list once and written under each of them.
//!
//! Children are written before their parent, so every offset points backwards and the
//! guest never needs a second pass.

use crate::{Error, Result};
use alloy_primitives::{keccak256, Bytes, B256, U256};
use alloy_rlp::Decodable;
use alloy_trie::{
    nodes::{RlpNode, TrieNode},
    Nibbles, TrieAccount, EMPTY_ROOT_HASH,
};
use std::collections::HashMap;

pub const MAGIC: [u8; 4] = *b"MZW\x01";
pub const HEADER_LEN: usize = 8;

pub const TAG_BRANCH: u8 = 0x00;
pub const TAG_EXT: u8 = 0x01;
pub const TAG_LEAF_ACCT: u8 = 0x02;
pub const TAG_LEAF_STORAGE: u8 = 0x03;
/// Doubles as the RLP string header of a 32-byte hash, which is why it is `0xa0`.
pub const TAG_DIGEST: u8 = 0xa0;

/// Offset of a node in the blob; `0` is the null node.
pub type Offset = u32;

#[derive(Clone, Copy)]
enum Kind {
    Account,
    Storage,
}

/// Append-only writer over the witness's node list.
pub struct OffsetTrie<'a> {
    /// Node preimages keyed by `keccak256(rlp)`, exactly as a branch child or an extension
    /// target refers to them. Nodes shorter than 32 bytes are inline and never looked up.
    nodes: HashMap<B256, &'a [u8]>,
    buf: Vec<u8>,
}

impl<'a> OffsetTrie<'a> {
    /// The blob for the account trie rooted at `state_root`, built from `state`.
    pub fn write(state: &'a [Bytes], state_root: B256) -> Result<Vec<u8>> {
        let mut trie = Self {
            nodes: state
                .iter()
                .map(|rlp| (keccak256(rlp), rlp.as_ref()))
                .collect(),
            buf: Vec::with_capacity(1 << 20),
        };
        trie.buf.extend_from_slice(&MAGIC);
        trie.buf.extend_from_slice(&0u32.to_le_bytes());
        let root = if state_root == EMPTY_ROOT_HASH {
            0
        } else {
            trie.emit_hash(state_root, Kind::Account)?
        };
        trie.buf[4..8].copy_from_slice(&root.to_le_bytes());
        Ok(trie.buf)
    }

    /// A node referenced by hash: its subtree, or a DIGEST when the witness lacks it.
    fn emit_hash(&mut self, hash: B256, kind: Kind) -> Result<Offset> {
        match self.nodes.get(&hash) {
            None => self.emit_digest(&hash),
            Some(rlp) => self.emit_rlp(hash, rlp, kind),
        }
    }

    /// A branch slot or extension target: a 32-byte hash, or an inline node under 32 bytes.
    /// `0` for the empty root.
    fn emit_child(&mut self, child: &RlpNode, kind: Kind) -> Result<Offset> {
        match child.as_hash() {
            Some(hash) if hash == EMPTY_ROOT_HASH => Ok(0),
            Some(hash) => self.emit_hash(hash, kind),
            // Inline nodes have no hash of their own; report errors under a zero hash.
            None => self.emit_rlp(B256::ZERO, child, kind),
        }
    }

    /// A materialised node: its children first, then the node itself.
    fn emit_rlp(&mut self, hash: B256, rlp: &[u8], kind: Kind) -> Result<Offset> {
        let bad = |reason: String| Error::Node { hash, reason };
        let node = TrieNode::decode(&mut &rlp[..]).map_err(|e| bad(format!("node rlp: {e}")))?;
        match node {
            TrieNode::EmptyRoot => Err(bad("empty-root node where a child was expected".into())),
            TrieNode::Branch(branch) => {
                let mut children = [0 as Offset; 16];
                for (nibble, child) in branch.as_ref().children() {
                    if let Some(child) = child {
                        children[usize::from(nibble)] = self.emit_child(child, kind)?;
                    }
                }
                let off = self.offset()?;
                self.buf.push(TAG_BRANCH);
                for child in children {
                    self.buf.extend_from_slice(&child.to_le_bytes());
                }
                Ok(off)
            }
            TrieNode::Extension(ext) => {
                let child = self.emit_child(&ext.child, kind)?;
                if child == 0 {
                    return Err(bad("extension into the empty root".into()));
                }
                let off = self.offset()?;
                self.buf.push(TAG_EXT);
                self.buf.extend_from_slice(&child.to_le_bytes());
                self.buf.extend_from_slice(&pack_path(&ext.key)?);
                Ok(off)
            }
            TrieNode::Leaf(leaf) => match kind {
                Kind::Account => {
                    let acct = TrieAccount::decode(&mut &leaf.value[..])
                        .map_err(|e| bad(format!("account value: {e}")))?;
                    let storage = if acct.storage_root == EMPTY_ROOT_HASH {
                        0
                    } else {
                        self.emit_hash(acct.storage_root, Kind::Storage)?
                    };
                    let mut nonce_balance = Vec::with_capacity(42);
                    alloy_rlp::Encodable::encode(&acct.nonce, &mut nonce_balance);
                    alloy_rlp::Encodable::encode(&acct.balance, &mut nonce_balance);
                    let nb_len = u8::try_from(nonce_balance.len())
                        .map_err(|_| Error::Overflow("nonce+balance rlp"))?;
                    let off = self.offset()?;
                    self.buf.push(TAG_LEAF_ACCT);
                    self.buf.extend_from_slice(&storage.to_le_bytes());
                    self.buf.push(TAG_DIGEST);
                    self.buf.extend_from_slice(acct.code_hash.as_slice());
                    self.buf.push(nb_len);
                    self.buf.extend_from_slice(&nonce_balance);
                    self.buf.extend_from_slice(&pack_path(&leaf.key)?);
                    Ok(off)
                }
                Kind::Storage => {
                    // The leaf RLP stores the value zeroless; the blob stores it widened.
                    let value = U256::decode(&mut &leaf.value[..])
                        .map_err(|e| bad(format!("storage value: {e}")))?;
                    let off = self.offset()?;
                    self.buf.push(TAG_LEAF_STORAGE);
                    self.buf.extend_from_slice(&value.to_be_bytes::<32>());
                    self.buf.extend_from_slice(&pack_path(&leaf.key)?);
                    Ok(off)
                }
            },
        }
    }

    fn emit_digest(&mut self, hash: &B256) -> Result<Offset> {
        let off = self.offset()?;
        self.buf.push(TAG_DIGEST);
        self.buf.extend_from_slice(hash.as_slice());
        Ok(off)
    }

    fn offset(&self) -> Result<Offset> {
        Offset::try_from(self.buf.len()).map_err(|_| Error::Overflow("blob offset"))
    }
}

/// `u8 nibble_count | packed nibbles`, first nibble in the high half of each byte, the
/// `category/core/nibble.h` convention.
pub fn pack_path(path: &Nibbles) -> Result<Vec<u8>> {
    let n = u8::try_from(path.len()).map_err(|_| Error::Overflow("path length"))?;
    let mut out = Vec::with_capacity(1 + path.len().div_ceil(2));
    out.push(n);
    let nibbles: Vec<u8> = path.to_vec();
    for pair in nibbles.chunks(2) {
        out.push((pair[0] << 4) | pair.get(1).copied().unwrap_or(0));
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloy_rlp::Encodable;

    #[test]
    fn path_packing_is_high_nibble_first_with_count_prefix() {
        let path = Nibbles::from_nibbles([0xa, 0xb, 0xc]);
        assert_eq!(pack_path(&path).unwrap(), vec![3, 0xab, 0xc0]);
        assert_eq!(pack_path(&Nibbles::default()).unwrap(), vec![0]);
    }

    #[test]
    fn empty_trie_is_header_only() {
        let blob = OffsetTrie::write(&[], EMPTY_ROOT_HASH).unwrap();
        assert_eq!(blob, b"MZW\x01\0\0\0\0".to_vec());
    }

    /// A root branch whose only child (slot 3) is a hash the witness does not carry.
    #[test]
    fn unrevealed_child_becomes_a_digest_written_before_the_branch() {
        let child = B256::repeat_byte(0x11);
        let mut branch = Vec::new();
        let mut payload = Vec::new();
        for slot in 0..17 {
            if slot == 3 {
                child.encode(&mut payload);
            } else {
                payload.push(alloy_rlp::EMPTY_STRING_CODE);
            }
        }
        alloy_rlp::Header {
            list: true,
            payload_length: payload.len(),
        }
        .encode(&mut branch);
        branch.extend_from_slice(&payload);
        let root = keccak256(&branch);

        let blob = OffsetTrie::write(&[Bytes::from(branch)], root).unwrap();
        // header | digest at 8 (33 bytes) | branch at 41
        assert_eq!(&blob[4..8], &41u32.to_le_bytes());
        assert_eq!(blob[8], TAG_DIGEST);
        assert_eq!(&blob[9..41], child.as_slice());
        assert_eq!(blob[41], TAG_BRANCH);
        assert_eq!(&blob[42 + 4 * 3..42 + 4 * 4], &8u32.to_le_bytes());
        assert_eq!(blob.len(), 41 + 1 + 64);
    }
}
