//! Inputs for the Monad executor's Ethereum mainnet guest.
//!
//! `from_rpc` talks to a stock reth node: `debug_getRawBlock` for the block's
//! exact RLP and `debug_executionWitness` for the pre-state, converted on the
//! host into the six-field Monad witness envelope by `monad_witness::convert`.
//! `from_witness` accepts an already-built envelope.

use std::time::Duration;

use alloy_consensus::{Block, TxEnvelope};
use alloy_primitives::{Bytes, U64};
use alloy_rlp::{Decodable, Header};
use alloy_rpc_types_debug::ExecutionWitness;
use anyhow::{ensure, Context, Result};
use async_trait::async_trait;
use http::{HeaderMap, HeaderName, HeaderValue};
use input_core::{BlockStats, ExecutionClient, InputStats};
use jsonrpsee::{core::client::ClientT, http_client::HttpClientBuilder, rpc_params};
use monad_witness::convert;
use zisk_sdk::ZiskStdin;

pub use input_core::RpcConfig;

// Leaves room for framing and RPC metadata below the cluster's 128 MiB limit.
const MAX_WITNESS_BYTES: usize = 64 * 1024 * 1024;

#[derive(Default)]
pub struct MonadClient;

impl MonadClient {
    /// Frame a raw Monad witness exactly once, without re-encoding its contents.
    /// Structural decoding is not execution or proof verification.
    pub fn from_witness(&self, monad_witness: &[u8]) -> Result<(ZiskStdin, BlockStats)> {
        let stats = witness_stats(monad_witness)?;
        let stdin = ZiskStdin::new();
        stdin.write_slice(monad_witness);
        Ok((
            stdin,
            BlockStats {
                chain_name: "Mainnet",
                block_number: stats.block_number,
                tx_count: stats.tx_count,
                gas_used: stats.gas_used,
            },
        ))
    }
}

#[async_trait]
impl ExecutionClient for MonadClient {
    fn name(&self) -> &'static str {
        "monad"
    }

    fn display_name(&self) -> &'static str {
        "Monad"
    }

    async fn from_rpc(
        &self,
        config: &RpcConfig,
        block_number: u64,
    ) -> Result<(ZiskStdin, BlockStats)> {
        let mut headers = HeaderMap::new();
        for (name, value) in &config.headers {
            let name =
                HeaderName::from_bytes(name.as_bytes()).context("Invalid RPC header name")?;
            let value = HeaderValue::from_str(value).context("Invalid RPC header value")?;
            headers.insert(name, value);
        }
        let client = HttpClientBuilder::default()
            .set_headers(headers)
            .request_timeout(Duration::from_secs(60))
            // A hex-encoded witness takes twice as many bytes, plus the JSON envelope.
            .max_response_size((2 * MAX_WITNESS_BYTES + 1024) as u32)
            .build(&config.url)
            .context("Failed to create Monad RPC client")?;

        // ExecutionClient has no startup hook. Check each fetch rather than cache
        // a chain identity across potentially different RpcConfig endpoints.
        let chain_id: U64 = client
            .request("eth_chainId", rpc_params![])
            .await
            .context("Failed to fetch eth_chainId")?;
        ensure!(
            chain_id == U64::from(1),
            "Monad guest supports Ethereum mainnet only; got chain ID {chain_id}"
        );

        let hex = format!("0x{block_number:x}");
        // The exact network bytes: the guest re-hashes them for the transactions root.
        let block_rlp: Bytes = client
            .request("debug_getRawBlock", rpc_params![hex.clone()])
            .await
            .with_context(|| format!("debug_getRawBlock({block_number})"))?;
        let got = block_number_of(&block_rlp)?;
        ensure!(
            got == block_number,
            "debug_getRawBlock returned block {got}, requested {block_number}"
        );
        let reth_witness: ExecutionWitness = client
            .request("debug_executionWitness", rpc_params![hex])
            .await
            .with_context(|| {
                format!("debug_executionWitness({block_number}): the node must be a reth archive node, or hold the parent state")
            })?;
        let monad_witness = convert(&reth_witness, &block_rlp)
            .context("converting the reth witness to the Monad format")?;
        let (stdin, stats) = self.from_witness(&monad_witness)?;
        ensure!(
            stats.block_number == block_number,
            "Witness block number {} does not match requested block {block_number}",
            stats.block_number
        );
        Ok((stdin, stats))
    }

    fn run(&self) {
        panic!("Monad native execution is not supported by input-monad; run the Monad guest ELF instead");
    }

    fn emits_hints(&self) -> bool {
        false
    }

    fn input_stats(&self, stdin: &ZiskStdin) -> Result<Option<InputStats>> {
        let bytes = stdin.read_data();
        Ok(Some(witness_stats(single_frame(&bytes)?)?))
    }
}

/// The header's `number` field of a raw block.
fn block_number_of(block_rlp: &[u8]) -> Result<u64> {
    let mut buf = block_rlp;
    let outer = Header::decode(&mut buf).context("Invalid block RLP")?;
    ensure!(outer.list, "Block RLP is not a list");
    let header = alloy_consensus::Header::decode(&mut buf).context("Invalid block header RLP")?;
    Ok(header.number)
}

fn witness_stats(mut monad_witness: &[u8]) -> Result<InputStats> {
    ensure!(
        monad_witness.len() <= MAX_WITNESS_BYTES,
        "Monad witness exceeds 64 MiB"
    );
    let mut fields =
        Header::decode_bytes(&mut monad_witness, true).context("Invalid Monad witness envelope")?;
    ensure!(
        monad_witness.is_empty(),
        "Trailing bytes after Monad witness envelope"
    );
    let mut block_rlp =
        Header::decode_bytes(&mut fields, false).context("Missing RLP block string")?;
    // The nodes field is opaque offset-trie data inside a list envelope, NOT
    // recursively encoded RLP nodes. Only check these five field boundaries.
    for name in [
        "nodes",
        "codes",
        "ancestor headers",
        "parent senders",
        "grandparent senders",
    ] {
        Header::decode_bytes(&mut fields, true)
            .with_context(|| format!("Invalid witness {name} field"))?;
    }
    ensure!(
        fields.is_empty(),
        "Monad witness must contain exactly six fields"
    );

    let block = Block::<TxEnvelope>::decode(&mut block_rlp).context("Invalid witness block RLP")?;
    ensure!(block_rlp.is_empty(), "Trailing bytes after witness block");
    Ok(InputStats {
        block_number: block.header.number,
        tx_count: block.body.transactions.len(),
        gas_used: block.header.gas_used,
    })
}

// Unlike a generic first-frame reader, Monad accepts exactly one frame. Check
// arithmetic, trailing data and padding before handing the payload to RLP.
fn single_frame(bytes: &[u8]) -> Result<&[u8]> {
    let prefix: [u8; 8] = bytes
        .get(..8)
        .context("Missing Monad input frame length")?
        .try_into()
        .unwrap();
    let len = usize::try_from(u64::from_le_bytes(prefix))
        .context("Input length exceeds address space")?;
    ensure!(len <= MAX_WITNESS_BYTES, "Monad witness exceeds 64 MiB");
    let end = 8usize
        .checked_add(len)
        .context("Input frame length overflow")?;
    let padded_end = end
        .checked_add(7)
        .context("Input padding length overflow")?
        & !7;
    ensure!(
        bytes.len() == padded_end,
        "Monad input must contain exactly one complete, padded frame"
    );
    ensure!(
        bytes[end..].iter().all(|&b| b == 0),
        "Nonzero Monad input padding"
    );
    Ok(&bytes[8..end])
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::{json, Value};
    use wiremock::{matchers::method, Mock, MockServer, Request, Respond, ResponseTemplate};

    /// Answers the three calls `from_rpc` makes, echoing the request id.
    struct RethMock {
        block: Value,
        reth_witness: Value,
    }

    impl Respond for RethMock {
        fn respond(&self, request: &Request) -> ResponseTemplate {
            let body: Value = serde_json::from_slice(&request.body).expect("json-rpc body");
            let result = match body["method"].as_str().expect("method") {
                "eth_chainId" => json!("0x1"),
                "debug_getRawBlock" => self.block.clone(),
                "debug_executionWitness" => self.reth_witness.clone(),
                other => panic!("unexpected RPC method {other}"),
            };
            ResponseTemplate::new(200).set_body_json(json!({
                "jsonrpc": "2.0",
                "id": body["id"],
                "result": result,
            }))
        }
    }

    /// End to end against a mocked reth, using the same fixtures as the golden test.
    #[tokio::test]
    async fn from_rpc_builds_the_fixture_witness() {
        let Ok(dir) = std::env::var("MONAD_WITNESS_FIXTURES") else {
            eprintln!("MONAD_WITNESS_FIXTURES unset; skipping");
            return;
        };
        let dir = std::path::Path::new(&dir);
        let Some(block) = std::fs::read_dir(dir)
            .expect("fixture dir")
            .filter_map(|e| e.ok())
            .find_map(|e| {
                e.file_name()
                    .to_str()?
                    .strip_suffix(".expected.witness")
                    .map(str::to_owned)
            })
        else {
            eprintln!("no fixtures; skipping");
            return;
        };
        let number: u64 = block.parse().expect("block number");
        let block_rlp = std::fs::read(dir.join(format!("{block}.block.rlp"))).unwrap();
        let reth_witness: Value =
            serde_json::from_slice(&std::fs::read(dir.join(format!("{block}.json"))).unwrap())
                .unwrap();
        let expected = std::fs::read(dir.join(format!("{block}.expected.witness"))).unwrap();

        let server = MockServer::start().await;
        Mock::given(method("POST"))
            .respond_with(RethMock {
                block: json!(Bytes::from(block_rlp)),
                reth_witness,
            })
            .mount(&server)
            .await;

        let (stdin, stats) = MonadClient
            .from_rpc(&RpcConfig::new(server.uri()), number)
            .await
            .expect("from_rpc");
        assert_eq!(stats.block_number, number);
        let bytes = stdin.read_data();
        assert!(
            single_frame(&bytes).unwrap() == expected.as_slice(),
            "witness differs from fixture"
        );
    }
}
