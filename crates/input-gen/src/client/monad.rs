use super::InputGenClient;
use crate::provider::ProviderKind;

// Monad consumes node-produced witnesses; no EEST conversion or native hints.
impl InputGenClient for input::MonadClient {
    fn supported_providers(&self) -> &'static [ProviderKind] {
        &[ProviderKind::Rpc]
    }
}
