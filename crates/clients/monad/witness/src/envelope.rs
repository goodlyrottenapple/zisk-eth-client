//! The Monad `.witness` envelope: a six-item RLP list.
//!
//! ```text
//! [ block_rlp            (string: the raw block)
//! , [ trie ]             (list whose payload is the raw trie bytes, not a string)
//! , [ code, ... ]        (strings)
//! , [ header, ... ]      (strings, each holding one header's RLP)
//! , [ ]                  (parent-block senders: empty for Ethereum traits)
//! , [ ]                  (grandparent-block senders: empty for Ethereum traits)
//! ]
//! ```
//!
//! Item 1 is the one irregularity: the trie is placed as the *payload* of a list header
//! so the guest can take a zero-copy view of it without an RLP string prefix of its own.

use alloy_primitives::Bytes;
use alloy_rlp::{Encodable, Header};

const EMPTY_LIST: u8 = 0xc0;

pub fn encode(block_rlp: &[u8], trie: &[u8], codes: &[Bytes], headers: &[Bytes]) -> Vec<u8> {
    let strings = |items: &[Bytes]| {
        let mut out = Vec::new();
        for item in items {
            item.encode(&mut out);
        }
        out
    };
    let codes_payload = strings(codes);
    let headers_payload = strings(headers);

    let items = [
        (false, block_rlp),
        (true, trie),
        (true, codes_payload.as_slice()),
        (true, headers_payload.as_slice()),
    ];
    let payload_length: usize = items
        .iter()
        .map(|(list, payload)| header_len(*list, payload.len()) + payload.len())
        .sum::<usize>()
        + 2;

    let mut out = Vec::with_capacity(header_len(true, payload_length) + payload_length);
    Header {
        list: true,
        payload_length,
    }
    .encode(&mut out);
    for (list, payload) in items {
        Header {
            list,
            payload_length: payload.len(),
        }
        .encode(&mut out);
        out.extend_from_slice(payload);
    }
    out.push(EMPTY_LIST);
    out.push(EMPTY_LIST);
    out
}

fn header_len(list: bool, payload_length: usize) -> usize {
    Header {
        list,
        payload_length,
    }
    .length()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn six_items_with_raw_trie_as_list_payload() {
        let block = Bytes::from(vec![0xc3, 0x01, 0x02, 0x03]);
        let trie = b"MZW\x01\0\0\0\0";
        let out = encode(
            &block,
            trie,
            &[Bytes::from(vec![0xaa])],
            &[Bytes::from(vec![0xc0])],
        );
        let expected = [
            vec![0xc0 + 5 + 9 + 3 + 3 + 2],           // outer list header
            vec![0x80 + 4, 0xc3, 0x01, 0x02, 0x03],   // block string
            [vec![0xc0 + 8], trie.to_vec()].concat(), // trie as list payload
            vec![0xc0 + 2, 0x81, 0xaa],               // codes
            vec![0xc0 + 2, 0x81, 0xc0],               // headers
            vec![0xc0, 0xc0],                         // senders
        ]
        .concat();
        assert_eq!(out, expected);
    }
}
