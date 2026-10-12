//! Byte-for-byte comparison against known-good `.witness` files.
//!
//! Set `MONAD_WITNESS_FIXTURES` to a directory holding, per block `N`, the reth witness
//! `N.json`, the raw block `N.block.rlp`, and `N.expected.witness`. Without the variable
//! the test is a no-op, so CI stays green while the fixtures live outside the repo.

use monad_witness::ExecutionWitness;
use std::path::Path;

fn convert_fixture(dir: &Path, block: &str) -> Option<Vec<u8>> {
    let block_rlp = std::fs::read(dir.join(format!("{block}.block.rlp"))).ok()?;
    let json = std::fs::read(dir.join(format!("{block}.json"))).ok()?;
    let witness: ExecutionWitness = serde_json::from_slice(&json).ok()?;
    monad_witness::convert(&witness, &block_rlp).ok()
}

#[test]
fn matches_expected_witnesses() {
    let Ok(dir) = std::env::var("MONAD_WITNESS_FIXTURES") else {
        eprintln!("MONAD_WITNESS_FIXTURES unset; skipping");
        return;
    };
    let dir = Path::new(&dir);
    let mut checked = 0;
    for entry in std::fs::read_dir(dir).expect("fixture dir") {
        let name = entry.expect("entry").file_name();
        let name = name.to_string_lossy();
        let Some(block) = name.strip_suffix(".expected.witness") else {
            continue;
        };
        let expected = std::fs::read(dir.join(&*name)).expect("expected witness");
        let got =
            convert_fixture(dir, block).unwrap_or_else(|| panic!("{block}: conversion failed"));
        assert!(
            got == expected,
            "{block}: output differs from expected ({} vs {} bytes)",
            got.len(),
            expected.len()
        );
        checked += 1;
    }
    eprintln!("{checked} fixtures compared");
}
