use alloy_primitives::B256;

/// Conversion failures. All of them mean the input is not a usable witness for the
/// requested block; none can be worked around by the converter.
#[derive(Debug)]
pub enum Error {
    /// The input could not be parsed (RLP).
    Parse(String),
    /// A node referenced by hash is malformed. Absent nodes are not errors: they become
    /// digests.
    Node { hash: B256, reason: String },
    /// The witness headers do not form an ascending, contiguous chain ending at the
    /// block's parent.
    Headers(String),
    /// A value is too large for the blob encoding (offsets are `u32`, paths `u8`-counted).
    Overflow(&'static str),
}

pub type Result<T> = core::result::Result<T, Error>;

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::Parse(msg) => write!(f, "parse error: {msg}"),
            Error::Node { hash, reason } => write!(f, "bad node {hash}: {reason}"),
            Error::Headers(msg) => write!(f, "header chain: {msg}"),
            Error::Overflow(what) => write!(f, "{what} exceeds the blob encoding"),
        }
    }
}

impl std::error::Error for Error {}
