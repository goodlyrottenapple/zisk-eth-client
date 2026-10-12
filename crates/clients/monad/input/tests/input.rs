use alloy_consensus::{
    constants::EMPTY_ROOT_HASH, Block, Header as BlockHeader, SignableTransaction, TxEip1559,
    TxEnvelope, TxLegacy,
};
use alloy_primitives::{keccak256, Bytes, Signature, U256};
use alloy_rlp::Header;
use input_core::{ExecutionClient, RpcConfig};
use input_monad::MonadClient;
use serde_json::{json, Value};
use wiremock::{
    matchers::{body_partial_json, header, method},
    Mock, MockServer, Request, ResponseTemplate,
};
use zisk_sdk::ZiskStdin;

fn list(payload: &[u8]) -> Vec<u8> {
    let mut out = Vec::new();
    Header {
        list: true,
        payload_length: payload.len(),
    }
    .encode(&mut out);
    out.extend_from_slice(payload);
    out
}

fn envelope(block: &[u8]) -> Vec<u8> {
    let mut fields = alloy_rlp::encode(block);
    // Offset-trie bytes are not nested RLP; rejecting this would reject real witnesses.
    fields.extend(list(&[0xff, 0, 1]));
    fields.extend([0xc0; 4]);
    list(&fields)
}

fn witness(number: u64) -> Vec<u8> {
    let signature = Signature::new(U256::from(1), U256::from(2), false);
    let mut block = Block::<TxEnvelope> {
        header: BlockHeader {
            number,
            gas_used: 42_000,
            ..Default::default()
        },
        ..Default::default()
    };
    block.body.transactions = vec![
        TxLegacy::default().into_signed(signature).into(),
        TxEip1559::default().into_signed(signature).into(),
    ];
    envelope(&alloy_rlp::encode(block))
}

#[test]
fn frames_raw_witness_and_recovers_metadata() {
    let raw = witness(25_815_003);
    let (stdin, stats) = MonadClient.from_witness(&raw).unwrap();
    assert_eq!(stats.chain_name, "Mainnet");
    assert_eq!(stats.block_number, 25_815_003);
    assert_eq!(stats.tx_count, 2);
    assert_eq!(stats.gas_used, 42_000);
    let data = stdin.read_data();
    assert_eq!(&data[..8], &(raw.len() as u64).to_le_bytes());
    assert_eq!(&data[8..8 + raw.len()], raw);
    assert_eq!(data.len() % 8, 0);
    assert!(data[8 + raw.len()..].iter().all(|b| *b == 0));
    let decoded = MonadClient.input_stats(&stdin).unwrap().unwrap();
    assert_eq!(decoded.block_number, stats.block_number);
    assert_eq!(decoded.tx_count, stats.tx_count);
    assert_eq!(decoded.gas_used, stats.gas_used);
    assert!(!MonadClient.emits_hints());
}

#[test]
fn rejects_malformed_envelopes_and_blocks() {
    let raw = witness(1);
    let mut trailing = raw.clone();
    trailing.push(0);
    let mut fields = raw.as_slice();
    let payload = Header::decode_bytes(&mut fields, true).unwrap();
    let mut extra = payload.to_vec();
    extra.push(0xc0);
    let mut wrong_type = payload.to_vec();
    *wrong_type.last_mut().unwrap() = 0x80;
    let block = alloy_rlp::encode(Block::<TxEnvelope>::default());
    let mut block_trailing = block.clone();
    block_trailing.push(0);
    for invalid in [
        vec![],
        vec![0x80],
        raw[..raw.len() - 1].to_vec(),
        trailing,
        list(&payload[..payload.len() - 1]),
        list(&extra),
        list(&wrong_type),
        envelope(&[]),
        envelope(&block_trailing),
    ] {
        assert!(MonadClient.from_witness(&invalid).is_err());
    }
}

#[test]
fn rejects_bad_framing_without_panicking() {
    let (stdin, _) = MonadClient.from_witness(&witness(1)).unwrap();
    let valid = stdin.read_data();
    let mut extra = valid.clone();
    extra.extend_from_slice(&[0; 8]);
    let mut oversized = valid.clone();
    oversized[..8].copy_from_slice(&u64::MAX.to_le_bytes());
    let mut bad_padding = vec![0; 16];
    bad_padding[0] = 1; // one payload byte, seven padding bytes
    bad_padding[8] = 0xc0;
    bad_padding[15] = 1;
    assert!(MonadClient
        .input_stats(&ZiskStdin::from_bytes(valid.clone()))
        .is_err());
    for bytes in [
        bad_padding,
        vec![],
        vec![0; 7],
        valid[..valid.len() - 1].to_vec(),
        extra,
        oversized,
    ] {
        let mut path = std::env::temp_dir();
        path.push(format!(
            "monad-input-frame-{}-{}.bin",
            std::process::id(),
            bytes.len()
        ));
        std::fs::write(&path, bytes).unwrap();
        let input = ZiskStdin::from_file(&path).unwrap();
        std::fs::remove_file(path).unwrap();
        assert!(MonadClient.input_stats(&input).is_err());
    }
}

async fn rpc(server: &MockServer, name: &str, response: Value, params: Value, calls: u64) {
    // jsonrpsee omits `params` for a no-argument request, which JSON-RPC permits.
    let expected = if params == json!([]) {
        json!({"method": name})
    } else {
        json!({"method": name, "params": params})
    };
    Mock::given(method("POST"))
        .and(header("authorization", "Bearer test-token"))
        .and(body_partial_json(expected))
        .respond_with(move |request: &Request| {
            let body: Value = serde_json::from_slice(&request.body).unwrap();
            let mut reply = response.clone();
            reply["jsonrpc"] = json!("2.0");
            reply["id"] = body["id"].clone();
            ResponseTemplate::new(200).set_body_json(reply)
        })
        .expect(calls)
        .mount(server)
        .await;
}

fn config(server: &MockServer) -> RpcConfig {
    RpcConfig::new(server.uri())
        .with_headers(vec![("Authorization".into(), "Bearer test-token".into())])
}

/// What a reth node answers for a synthetic block `number` with no transactions on top
/// of an empty pre-state: the `debug_getRawBlock` result and the `debug_executionWitness`
/// result, which only needs the parent header.
fn reth_fixture(number: u64) -> (Value, Value) {
    let parent = BlockHeader {
        number: number - 1,
        state_root: EMPTY_ROOT_HASH,
        ..Default::default()
    };
    let parent_rlp = alloy_rlp::encode(&parent);
    let block = Block::<TxEnvelope> {
        header: BlockHeader {
            number,
            parent_hash: keccak256(&parent_rlp),
            gas_used: 42_000,
            ..Default::default()
        },
        ..Default::default()
    };
    let reth_witness = json!({
        "state": [],
        "codes": [],
        "keys": [],
        "headers": [Bytes::from(parent_rlp)],
    });
    (json!(Bytes::from(alloy_rlp::encode(block))), reth_witness)
}

#[tokio::test]
async fn fetches_block_and_reth_witness_with_quantity_parameters_and_auth_headers() {
    let server = MockServer::start().await;
    let (block, reth_witness) = reth_fixture(42);
    rpc(
        &server,
        "eth_chainId",
        json!({"result": "0x1"}),
        json!([]),
        1,
    )
    .await;
    rpc(
        &server,
        "debug_getRawBlock",
        json!({"result": block}),
        json!(["0x2a"]),
        1,
    )
    .await;
    rpc(
        &server,
        "debug_executionWitness",
        json!({"result": reth_witness}),
        json!(["0x2a"]),
        1,
    )
    .await;
    let (stdin, stats) = MonadClient.from_rpc(&config(&server), 42).await.unwrap();
    assert_eq!(stats.block_number, 42);
    assert_eq!(stats.tx_count, 0);
    assert_eq!(stats.gas_used, 42_000);
    // The frame holds a Monad envelope carrying the raw block.
    let input_stats = MonadClient.input_stats(&stdin).unwrap().unwrap();
    assert_eq!(input_stats.block_number, 42);
    assert_eq!(input_stats.gas_used, 42_000);
}

#[tokio::test]
async fn rejects_other_chains_before_fetching_witness() {
    let server = MockServer::start().await;
    rpc(
        &server,
        "eth_chainId",
        json!({"result": "0x279f"}),
        json!([]),
        1,
    )
    .await;
    let error = MonadClient
        .from_rpc(&config(&server), 42)
        .await
        .err()
        .unwrap();
    assert!(error.to_string().contains("Ethereum mainnet only"));
    assert_eq!(server.received_requests().await.unwrap().len(), 1);
}

#[tokio::test]
async fn rejects_wrong_block_bad_witnesses_and_rpc_errors() {
    let (block, reth_witness) = reth_fixture(42);
    let (other_block, _) = reth_fixture(43);
    // (debug_getRawBlock reply, debug_executionWitness reply, expected witness calls, message)
    let cases = [
        (
            json!({"result": other_block}),
            json!({"result": reth_witness}),
            0,
            "returned block 43, requested 42",
        ),
        (
            json!({"result": "0xc0"}),
            json!({"result": reth_witness}),
            0,
            "Invalid block header RLP",
        ),
        (
            // A Monad envelope where a reth witness object is expected.
            json!({"result": block}),
            json!({"result": "0xc0"}),
            1,
            "debug_executionWitness(42)",
        ),
        (
            json!({"result": block}),
            json!({"result": {"state": [], "codes": [], "keys": [], "headers": []}}),
            1,
            "no headers",
        ),
        (
            json!({"result": block}),
            json!({"error": {"code": -32000, "message": "witness not available"}}),
            1,
            "witness not available",
        ),
    ];
    for (block_reply, witness_reply, witness_calls, message) in cases {
        let server = MockServer::start().await;
        rpc(
            &server,
            "eth_chainId",
            json!({"result": "0x1"}),
            json!([]),
            1,
        )
        .await;
        rpc(
            &server,
            "debug_getRawBlock",
            block_reply,
            json!(["0x2a"]),
            1,
        )
        .await;
        rpc(
            &server,
            "debug_executionWitness",
            witness_reply,
            json!(["0x2a"]),
            witness_calls,
        )
        .await;
        let error = MonadClient
            .from_rpc(&config(&server), 42)
            .await
            .err()
            .unwrap();
        assert!(format!("{error:#}").contains(message), "{error:#}");
    }
}

#[tokio::test]
async fn rejects_invalid_header_configuration() {
    let config = RpcConfig::new("http://127.0.0.1:1")
        .with_headers(vec![("Authorization".into(), "invalid\nvalue".into())]);
    let error = MonadClient.from_rpc(&config, 42).await.err().unwrap();
    assert!(error.to_string().contains("Invalid RPC header value"));
}

// Optional acceptance test against a real node-produced witness, without committing
// a megabyte-scale fixture. MONAD_WITNESS points to raw bytes, not ZiskStdin framing.
#[test]
#[ignore = "requires MONAD_WITNESS and MONAD_BLOCK_NUMBER"]
fn real_witness() {
    let bytes = std::fs::read(std::env::var("MONAD_WITNESS").unwrap()).unwrap();
    let expected: u64 = std::env::var("MONAD_BLOCK_NUMBER")
        .unwrap()
        .parse()
        .unwrap();
    let (stdin, stats) = MonadClient.from_witness(&bytes).unwrap();
    assert_eq!(stats.block_number, expected);
    assert_eq!(
        MonadClient.input_stats(&stdin).unwrap().unwrap().tx_count,
        stats.tx_count
    );
}
