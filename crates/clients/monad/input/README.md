# Monad input adapter

`input-monad` supplies Ethereum mainnet witnesses to the Monad ZisK guest.
It does not execute the guest natively or emit hints. The guest ELF is not
embedded: build it in the Monad repo and pass it to the host with `--elf`;
see the [guest guide](https://github.com/category-labs/monad/blob/main/zkvm/README.md#zisk)
for build prerequisites and instructions.

## RPC contract

The adapter talks to an unmodified reth node (an archive node, or one whose witness
window still holds the block's parent state). Each fetch makes three calls, with the
configured RPC headers applied to all of them:

```json
{"jsonrpc":"2.0","id":1,"method":"eth_chainId","params":[]}
{"jsonrpc":"2.0","id":2,"method":"debug_getRawBlock","params":["0x189e85b"]}
{"jsonrpc":"2.0","id":3,"method":"debug_executionWitness","params":["0x189e85b"]}
```

The chain id must be `0x1`. `debug_getRawBlock` supplies the block's exact RLP bytes,
which the guest re-hashes for the transactions root. The reth witness object (`state`,
`codes`, `keys`, `headers`) is converted by the `monad-witness` crate into the six-field
Monad RLP witness envelope, before ZisK framing:

1. RLP-encoded Ethereum block, wrapped as an RLP byte string.
2. Offset-trie node bytes (`MZW\x01` blob), wrapped as an RLP list.
3. List of contract bytecodes.
4. List of ancestor headers, each wrapped as an RLP byte string.
5. Parent sender/authority list (empty for Ethereum).
6. Grandparent sender/authority list (empty for Ethereum).

RPC errors (including an unavailable/pruned witness) are returned to the caller
without retries. Requests time out after 60 seconds; witnesses are limited to 64 MiB.

The adapter validates the envelope boundaries, decodes the block, and checks
its number against the request. Guest execution performs state/consensus
validation. Transaction count comes from the body; gas used and block number
come from the header. The adapter writes one `ZiskStdin::write_slice` frame.

`ExecutionClient::from_rpc` accepts only a block number. It cannot check a
subscribed head's hash; a reorg at the same height remains a caller/interface
integration concern. No block-hash consistency guarantee is implied here.

## Use

Initialize workspace submodules, then build the optional client:

```sh
cargo run --release -p input-gen --features monad -- \
  -c monad rpc -u http://localhost:8545 -b 25815003
```

For Rust callers, enable `input`'s `monad` feature and use
`create_client(Client::Monad)`. `MonadClient::from_witness` also accepts raw
witness bytes directly. `input_stats` expects exactly one length-prefixed,
zero-padded frame, as produced by this adapter. Existing framed `.bin` files
must not be passed to `from_witness` and framed a second time.

The host loads the Monad guest ELF from disk:

```sh
cargo run --release -p host --features monad -- \
  --action execute --emulator stateless-validator -c monad \
  --elf path/to/monad-zkvm-zisk.elf -i monad-inputs
```

`--elf` is required for `monad`; the host refuses to run without it. The ELF is
built in the Monad repo (`zkvm/zisk`) following the guest guide linked above,
which needs the ZisK Rust toolchain, CMake, Boost headers and a RISC-V GCC
toolchain. Building the host itself needs none of those. No prebuilt ELF is
checked into Git.

Monad has no EEST input conversion or native hints support. Do not use
`--gen-hints`. Building the host may also invoke the existing other-client
guest builds; the input adapter itself requires no C++/RISC-V toolchain.

## Validation

```sh
cargo test -p input-monad
MONAD_WITNESS=/path/to/raw.witness MONAD_BLOCK_NUMBER=25815003 \
  cargo test -p input-monad real_witness -- --ignored
```

The synthetic tests cover legacy/typed transactions, opaque trie bytes,
framing and metadata, invalid inputs, RPC headers and request parameters,
wrong chains/blocks, malformed blocks and witnesses and JSON-RPC
errors. With `MONAD_WITNESS_FIXTURES` set (see `monad-witness`), a mocked reth
replays a real block's witness through `from_rpc` and the result is compared
byte for byte. The optional real-witness check parses and frames a
node-produced witness; it does not execute or prove it.
