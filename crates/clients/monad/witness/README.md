# monad-witness

Turns a reth `debug_executionWitness` result plus the block's raw RLP into a Monad zkVM
`.witness`: the six-field RLP envelope carrying the `MZW\x01` offset-trie blob that the
guest's `PartialTrieDb` reads in place (monad repo, `category/execution/ethereum/db/
offset_trie.hpp`).

`convert(&ExecutionWitness, block_rlp)`, one module per stage: `headers` (chain
validation; the parent header's state root is the trie entry point), `offset_trie` (walks
the flat node list from that root and writes the post-order blob directly, a DIGEST
wherever the witness stops), `envelope`. `input-monad` calls `convert` from `from_rpc`; the
`monad-witness` binary converts a saved witness JSON + raw block offline.

Validation is by execution, not by node-set comparison: run the output through the monad
repo's `build/zkvm/guest/monad-zkvm-x86-test-runner` and compare the printed block hash.
`tests/golden.rs` byte-compares against fixtures named by `MONAD_WITNESS_FIXTURES`.
