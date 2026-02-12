// Binary serialization format for BlockchainTest data.
// Used by the converter (write path) and execute_blockchain_test (read path).
#pragma once

#include "../evmone_test/blockchaintest.hpp"
#include <cstring>
#include <vector>

namespace binary_format
{

// ============================================================================
// BinaryWriter
// ============================================================================

class BinaryWriter
{
    std::vector<uint8_t>& buf_;

public:
    explicit BinaryWriter(std::vector<uint8_t>& buf) : buf_(buf) {}

    void write_raw(const void* data, size_t len)
    {
        const auto* p = static_cast<const uint8_t*>(data);
        buf_.insert(buf_.end(), p, p + len);
    }

    void write_u8(uint8_t v) { buf_.push_back(v); }
    void write_bool(bool v) { write_u8(v ? 1 : 0); }

    void write_u32(uint32_t v) { write_raw(&v, sizeof(v)); }
    void write_i32(int32_t v) { write_raw(&v, sizeof(v)); }
    void write_u64(uint64_t v) { write_raw(&v, sizeof(v)); }
    void write_i64(int64_t v) { write_raw(&v, sizeof(v)); }

    void write_address(const evmc::address& addr) { write_raw(addr.bytes, 20); }
    void write_bytes32(const evmc::bytes32& b) { write_raw(b.bytes, 32); }
    void write_bloom(const evmone::state::BloomFilter& bl) { write_raw(bl.bytes, 256); }

    void write_uint256(const intx::uint256& v)
    {
        uint8_t tmp[32];
        intx::le::unsafe::store(tmp, v);
        write_raw(tmp, 32);
    }

    void write_bytes(const evmc::bytes& b)
    {
        write_u32(static_cast<uint32_t>(b.size()));
        if (!b.empty())
            write_raw(b.data(), b.size());
    }

    void write_string(const std::string& s)
    {
        write_u32(static_cast<uint32_t>(s.size()));
        if (!s.empty())
            write_raw(s.data(), s.size());
    }

    template <typename T>
    void write_optional(const std::optional<T>& opt, void (BinaryWriter::*write_fn)(T))
    {
        if (opt.has_value())
        {
            write_u8(1);
            (this->*write_fn)(*opt);
        }
        else
        {
            write_u8(0);
        }
    }

    void write_optional_u64(const std::optional<uint64_t>& opt)
    {
        write_optional(opt, &BinaryWriter::write_u64);
    }

    void write_optional_uint256(const std::optional<intx::uint256>& opt)
    {
        if (opt.has_value())
        {
            write_u8(1);
            write_uint256(*opt);
        }
        else
        {
            write_u8(0);
        }
    }

    void write_optional_address(const std::optional<evmc::address>& opt)
    {
        if (opt.has_value())
        {
            write_u8(1);
            write_address(*opt);
        }
        else
        {
            write_u8(0);
        }
    }
};

// ============================================================================
// BinaryReader
// ============================================================================

class BinaryReader
{
    const uint8_t* data_;
    size_t size_;
    size_t pos_ = 0;

public:
    BinaryReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    void read_raw(void* dst, size_t len)
    {
        std::memcpy(dst, data_ + pos_, len);
        pos_ += len;
    }

    uint8_t read_u8()
    {
        return data_[pos_++];
    }

    bool read_bool() { return read_u8() != 0; }

    uint32_t read_u32()
    {
        uint32_t v;
        read_raw(&v, sizeof(v));
        return v;
    }

    int32_t read_i32()
    {
        int32_t v;
        read_raw(&v, sizeof(v));
        return v;
    }

    uint64_t read_u64()
    {
        uint64_t v;
        read_raw(&v, sizeof(v));
        return v;
    }

    int64_t read_i64()
    {
        int64_t v;
        read_raw(&v, sizeof(v));
        return v;
    }

    evmc::address read_address()
    {
        evmc::address addr{};
        read_raw(addr.bytes, 20);
        return addr;
    }

    evmc::bytes32 read_bytes32()
    {
        evmc::bytes32 b{};
        read_raw(b.bytes, 32);
        return b;
    }

    evmone::state::BloomFilter read_bloom()
    {
        evmone::state::BloomFilter bl{};
        read_raw(bl.bytes, 256);
        return bl;
    }

    intx::uint256 read_uint256()
    {
        uint8_t tmp[32];
        read_raw(tmp, 32);
        return intx::le::unsafe::load<intx::uint256>(tmp);
    }

    evmc::bytes read_bytes()
    {
        uint32_t len = read_u32();
        evmc::bytes b(len, 0);
        if (len > 0)
            read_raw(b.data(), len);
        return b;
    }

    std::string read_string()
    {
        uint32_t len = read_u32();
        std::string s(len, '\0');
        if (len > 0)
            read_raw(s.data(), len);
        return s;
    }

    std::optional<uint64_t> read_optional_u64()
    {
        if (read_u8())
            return read_u64();
        return std::nullopt;
    }

    std::optional<intx::uint256> read_optional_uint256()
    {
        if (read_u8())
            return read_uint256();
        return std::nullopt;
    }

    std::optional<evmc::address> read_optional_address()
    {
        if (read_u8())
            return read_address();
        return std::nullopt;
    }

    size_t position() const { return pos_; }
    size_t remaining() const { return size_ - pos_; }
};

// ============================================================================
// Write functions for each struct
// ============================================================================

inline void write_revision_schedule(BinaryWriter& w, const evmone::test::RevisionSchedule& rs)
{
    w.write_i32(static_cast<int32_t>(rs.genesis_rev));
    w.write_i32(static_cast<int32_t>(rs.final_rev));
    w.write_i64(rs.transition_time);
}

inline void write_test_account(BinaryWriter& w, const evmone::test::TestAccount& acc)
{
    w.write_u64(acc.nonce);
    w.write_uint256(acc.balance);
    w.write_u32(static_cast<uint32_t>(acc.storage.size()));
    for (const auto& [key, val] : acc.storage)
    {
        w.write_bytes32(key);
        w.write_bytes32(val);
    }
    w.write_bytes(acc.code);
}

inline void write_test_state(BinaryWriter& w, const evmone::test::TestState& state)
{
    w.write_u32(static_cast<uint32_t>(state.size()));
    for (const auto& [addr, acc] : state)
    {
        w.write_address(addr);
        write_test_account(w, acc);
    }
}

inline void write_ommer(BinaryWriter& w, const evmone::state::Ommer& o)
{
    w.write_address(o.beneficiary);
    w.write_u32(o.delta);
}

inline void write_withdrawal(BinaryWriter& w, const evmone::state::Withdrawal& wd)
{
    w.write_u64(wd.index);
    w.write_u64(wd.validator_index);
    w.write_address(wd.recipient);
    w.write_u64(wd.amount_in_gwei);
}

inline void write_block_info(BinaryWriter& w, const evmone::state::BlockInfo& bi)
{
    w.write_i64(bi.number);
    w.write_i64(bi.timestamp);
    w.write_i64(bi.parent_timestamp);
    w.write_i64(bi.gas_limit);
    w.write_address(bi.coinbase);
    w.write_i64(bi.difficulty);
    w.write_i64(bi.parent_difficulty);
    w.write_bytes32(bi.parent_ommers_hash);
    w.write_bytes32(bi.prev_randao);
    w.write_bytes32(bi.parent_beacon_block_root);
    w.write_u64(bi.base_fee);
    w.write_optional_u64(bi.blob_gas_used);
    w.write_optional_u64(bi.excess_blob_gas);
    w.write_optional_uint256(bi.blob_base_fee);
    w.write_u32(static_cast<uint32_t>(bi.ommers.size()));
    for (const auto& o : bi.ommers)
        write_ommer(w, o);
    w.write_u32(static_cast<uint32_t>(bi.withdrawals.size()));
    for (const auto& wd : bi.withdrawals)
        write_withdrawal(w, wd);
}

inline void write_authorization(BinaryWriter& w, const evmone::state::Authorization& auth)
{
    w.write_uint256(auth.chain_id);
    w.write_address(auth.addr);
    w.write_u64(auth.nonce);
    w.write_optional_address(auth.signer);
    w.write_uint256(auth.r);
    w.write_uint256(auth.s);
    w.write_uint256(auth.v);
}

inline void write_access_list(BinaryWriter& w, const evmone::state::AccessList& al)
{
    w.write_u32(static_cast<uint32_t>(al.size()));
    for (const auto& [addr, keys] : al)
    {
        w.write_address(addr);
        w.write_u32(static_cast<uint32_t>(keys.size()));
        for (const auto& key : keys)
            w.write_bytes32(key);
    }
}

inline void write_transaction(BinaryWriter& w, const evmone::state::Transaction& tx)
{
    w.write_u8(static_cast<uint8_t>(tx.type));
    w.write_bytes(tx.data);
    w.write_i64(tx.gas_limit);
    w.write_uint256(tx.max_gas_price);
    w.write_uint256(tx.max_priority_gas_price);
    w.write_uint256(tx.max_blob_gas_price);
    w.write_address(tx.sender);
    w.write_optional_address(tx.to);
    w.write_uint256(tx.value);
    write_access_list(w, tx.access_list);
    w.write_u32(static_cast<uint32_t>(tx.blob_hashes.size()));
    for (const auto& h : tx.blob_hashes)
        w.write_bytes32(h);
    w.write_u64(tx.chain_id);
    w.write_u64(tx.nonce);
    w.write_uint256(tx.r);
    w.write_uint256(tx.s);
    w.write_u8(tx.v);
    w.write_u32(static_cast<uint32_t>(tx.authorization_list.size()));
    for (const auto& auth : tx.authorization_list)
        write_authorization(w, auth);
}

inline void write_block_header(BinaryWriter& w, const evmone::test::BlockHeader& bh)
{
    w.write_bytes32(bh.parent_hash);
    w.write_address(bh.coinbase);
    w.write_bytes32(bh.state_root);
    w.write_bytes32(bh.receipts_root);
    w.write_bloom(bh.logs_bloom);
    w.write_i64(bh.difficulty);
    w.write_bytes32(bh.prev_randao);
    w.write_i64(bh.block_number);
    w.write_i64(bh.gas_limit);
    w.write_i64(bh.gas_used);
    w.write_i64(bh.timestamp);
    w.write_bytes(bh.extra_data);
    w.write_u64(bh.base_fee_per_gas);
    w.write_bytes32(bh.hash);
    w.write_bytes32(bh.transactions_root);
    w.write_bytes32(bh.withdrawal_root);
    w.write_bytes32(bh.parent_beacon_block_root);
    w.write_optional_u64(bh.blob_gas_used);
    w.write_optional_u64(bh.excess_blob_gas);
    w.write_bytes32(bh.requests_hash);
}

inline void write_test_block(BinaryWriter& w, const evmone::test::TestBlock& tb)
{
    write_block_info(w, tb.block_info);
    w.write_u32(static_cast<uint32_t>(tb.transactions.size()));
    for (const auto& tx : tb.transactions)
        write_transaction(w, tx);
    w.write_bool(tb.valid);
    write_block_header(w, tb.expected_block_header);
}

inline void write_expectation(
    BinaryWriter& w, const evmone::test::BlockchainTest::Expectation& exp)
{
    w.write_bytes32(exp.last_block_hash);
    if (std::holds_alternative<evmone::test::TestState>(exp.post_state))
    {
        w.write_u8(0);  // tag: TestState
        write_test_state(w, std::get<evmone::test::TestState>(exp.post_state));
    }
    else
    {
        w.write_u8(1);  // tag: hash256
        w.write_bytes32(std::get<evmone::hash256>(exp.post_state));
    }
}

inline void write_blockchain_test(BinaryWriter& w, const evmone::test::BlockchainTest& bt)
{
    w.write_string(bt.name);
    write_revision_schedule(w, bt.rev);
    write_test_state(w, bt.pre_state);
    write_block_header(w, bt.genesis_block_header);
    w.write_u32(static_cast<uint32_t>(bt.test_blocks.size()));
    for (const auto& tb : bt.test_blocks)
        write_test_block(w, tb);
    write_expectation(w, bt.expectation);
}

/// Serialize a vector of blockchain tests to binary.
inline std::vector<uint8_t> serialize_blockchain_tests(
    const std::vector<evmone::test::BlockchainTest>& tests)
{
    std::vector<uint8_t> buf;
    BinaryWriter w(buf);
    w.write_u32(static_cast<uint32_t>(tests.size()));
    for (const auto& t : tests)
        write_blockchain_test(w, t);
    return buf;
}

// ============================================================================
// Read functions for each struct
// ============================================================================

inline evmone::test::RevisionSchedule read_revision_schedule(BinaryReader& r)
{
    evmone::test::RevisionSchedule rs;
    rs.genesis_rev = static_cast<evmc_revision>(r.read_i32());
    rs.final_rev = static_cast<evmc_revision>(r.read_i32());
    rs.transition_time = r.read_i64();
    return rs;
}

inline evmone::test::TestAccount read_test_account(BinaryReader& r)
{
    evmone::test::TestAccount acc;
    acc.nonce = r.read_u64();
    acc.balance = r.read_uint256();
    uint32_t storage_count = r.read_u32();
    for (uint32_t i = 0; i < storage_count; ++i)
    {
        auto key = r.read_bytes32();
        auto val = r.read_bytes32();
        acc.storage[key] = val;
    }
    acc.code = r.read_bytes();
    return acc;
}

inline evmone::test::TestState read_test_state(BinaryReader& r)
{
    evmone::test::TestState state;
    uint32_t count = r.read_u32();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto addr = r.read_address();
        state[addr] = read_test_account(r);
    }
    return state;
}

inline evmone::state::Ommer read_ommer(BinaryReader& r)
{
    evmone::state::Ommer o;
    o.beneficiary = r.read_address();
    o.delta = r.read_u32();
    return o;
}

inline evmone::state::Withdrawal read_withdrawal(BinaryReader& r)
{
    evmone::state::Withdrawal wd;
    wd.index = r.read_u64();
    wd.validator_index = r.read_u64();
    wd.recipient = r.read_address();
    wd.amount_in_gwei = r.read_u64();
    return wd;
}

inline evmone::state::BlockInfo read_block_info(BinaryReader& r)
{
    evmone::state::BlockInfo bi;
    bi.number = r.read_i64();
    bi.timestamp = r.read_i64();
    bi.parent_timestamp = r.read_i64();
    bi.gas_limit = r.read_i64();
    bi.coinbase = r.read_address();
    bi.difficulty = r.read_i64();
    bi.parent_difficulty = r.read_i64();
    bi.parent_ommers_hash = r.read_bytes32();
    bi.prev_randao = r.read_bytes32();
    bi.parent_beacon_block_root = r.read_bytes32();
    bi.base_fee = r.read_u64();
    bi.blob_gas_used = r.read_optional_u64();
    bi.excess_blob_gas = r.read_optional_u64();
    bi.blob_base_fee = r.read_optional_uint256();
    uint32_t ommer_count = r.read_u32();
    for (uint32_t i = 0; i < ommer_count; ++i)
        bi.ommers.push_back(read_ommer(r));
    uint32_t wd_count = r.read_u32();
    for (uint32_t i = 0; i < wd_count; ++i)
        bi.withdrawals.push_back(read_withdrawal(r));
    return bi;
}

inline evmone::state::Authorization read_authorization(BinaryReader& r)
{
    evmone::state::Authorization auth;
    auth.chain_id = r.read_uint256();
    auth.addr = r.read_address();
    auth.nonce = r.read_u64();
    auth.signer = r.read_optional_address();
    auth.r = r.read_uint256();
    auth.s = r.read_uint256();
    auth.v = r.read_uint256();
    return auth;
}

inline evmone::state::AccessList read_access_list(BinaryReader& r)
{
    evmone::state::AccessList al;
    uint32_t count = r.read_u32();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto addr = r.read_address();
        uint32_t key_count = r.read_u32();
        std::vector<evmc::bytes32> keys;
        for (uint32_t j = 0; j < key_count; ++j)
            keys.push_back(r.read_bytes32());
        al.emplace_back(addr, std::move(keys));
    }
    return al;
}

inline evmone::state::Transaction read_transaction(BinaryReader& r)
{
    evmone::state::Transaction tx;
    tx.type = static_cast<evmone::state::Transaction::Type>(r.read_u8());
    tx.data = r.read_bytes();
    tx.gas_limit = r.read_i64();
    tx.max_gas_price = r.read_uint256();
    tx.max_priority_gas_price = r.read_uint256();
    tx.max_blob_gas_price = r.read_uint256();
    tx.sender = r.read_address();
    tx.to = r.read_optional_address();
    tx.value = r.read_uint256();
    tx.access_list = read_access_list(r);
    uint32_t blob_count = r.read_u32();
    for (uint32_t i = 0; i < blob_count; ++i)
        tx.blob_hashes.push_back(r.read_bytes32());
    tx.chain_id = r.read_u64();
    tx.nonce = r.read_u64();
    tx.r = r.read_uint256();
    tx.s = r.read_uint256();
    tx.v = r.read_u8();
    uint32_t auth_count = r.read_u32();
    for (uint32_t i = 0; i < auth_count; ++i)
        tx.authorization_list.push_back(read_authorization(r));
    return tx;
}

inline evmone::test::BlockHeader read_block_header(BinaryReader& r)
{
    evmone::test::BlockHeader bh;
    bh.parent_hash = r.read_bytes32();
    bh.coinbase = r.read_address();
    bh.state_root = r.read_bytes32();
    bh.receipts_root = r.read_bytes32();
    bh.logs_bloom = r.read_bloom();
    bh.difficulty = r.read_i64();
    bh.prev_randao = r.read_bytes32();
    bh.block_number = r.read_i64();
    bh.gas_limit = r.read_i64();
    bh.gas_used = r.read_i64();
    bh.timestamp = r.read_i64();
    bh.extra_data = r.read_bytes();
    bh.base_fee_per_gas = r.read_u64();
    bh.hash = r.read_bytes32();
    bh.transactions_root = r.read_bytes32();
    bh.withdrawal_root = r.read_bytes32();
    bh.parent_beacon_block_root = r.read_bytes32();
    bh.blob_gas_used = r.read_optional_u64();
    bh.excess_blob_gas = r.read_optional_u64();
    bh.requests_hash = r.read_bytes32();
    return bh;
}

inline evmone::test::TestBlock read_test_block(BinaryReader& r)
{
    evmone::test::TestBlock tb;
    tb.block_info = read_block_info(r);
    uint32_t tx_count = r.read_u32();
    for (uint32_t i = 0; i < tx_count; ++i)
        tb.transactions.push_back(read_transaction(r));
    tb.valid = r.read_bool();
    tb.expected_block_header = read_block_header(r);
    return tb;
}

inline evmone::test::BlockchainTest::Expectation read_expectation(BinaryReader& r)
{
    evmone::test::BlockchainTest::Expectation exp;
    exp.last_block_hash = r.read_bytes32();
    uint8_t tag = r.read_u8();
    if (tag == 0)
        exp.post_state = read_test_state(r);
    else
        exp.post_state = r.read_bytes32();
    return exp;
}

inline evmone::test::BlockchainTest read_blockchain_test(BinaryReader& r)
{
    evmone::test::BlockchainTest bt;
    bt.name = r.read_string();
    bt.rev = read_revision_schedule(r);
    bt.pre_state = read_test_state(r);
    bt.genesis_block_header = read_block_header(r);
    uint32_t block_count = r.read_u32();
    for (uint32_t i = 0; i < block_count; ++i)
        bt.test_blocks.push_back(read_test_block(r));
    bt.expectation = read_expectation(r);
    return bt;
}

/// Deserialize a vector of blockchain tests from binary.
inline std::vector<evmone::test::BlockchainTest> deserialize_blockchain_tests(
    const uint8_t* data, size_t size)
{
    BinaryReader r(data, size);
    uint32_t count = r.read_u32();
    std::vector<evmone::test::BlockchainTest> tests;
    tests.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        tests.push_back(read_blockchain_test(r));
    return tests;
}

}  // namespace binary_format
