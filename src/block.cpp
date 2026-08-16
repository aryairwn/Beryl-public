// © Arya Irawan — 10 August 2026

#include "block.h"
#include "blockhash.h"

#include <cstdint>
#include <limits>
#include <string>

// ============================================================
// CANONICAL SERIALIZATION HELPERS
// ============================================================

static void WriteU32(
    std::string& out,
    uint32_t value
)
{
    out.push_back(
        static_cast<char>(value & 0xff)
    );

    out.push_back(
        static_cast<char>((value >> 8) & 0xff)
    );

    out.push_back(
        static_cast<char>((value >> 16) & 0xff)
    );

    out.push_back(
        static_cast<char>((value >> 24) & 0xff)
    );
}

static void WriteI32(
    std::string& out,
    int32_t value
)
{
    WriteU32(
        out,
        static_cast<uint32_t>(value)
    );
}

static void WriteU64(
    std::string& out,
    uint64_t value
)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(
            static_cast<char>(
                (value >> (8 * i)) & 0xff
            )
        );
    }
}

static void WriteString(
    std::string& out,
    const std::string& value
)
{
    if (value.size() >
        std::numeric_limits<uint32_t>::max())
    {
        return;
    }

    WriteU32(
        out,
        static_cast<uint32_t>(value.size())
    );

    out.append(value);
}

// ============================================================
// CANONICAL TRANSACTION SERIALIZATION
// ============================================================

static void SerializeTransaction(
    std::string& out,
    const BerylTransaction& tx
)
{
    // Native transaction type.
    WriteU32(
        out,
        static_cast<uint32_t>(tx.type)
    );

    // Contract payload.
    WriteU32(
        out,
        static_cast<uint32_t>(tx.contract.type)
    );

    WriteU32(
        out,
        static_cast<uint32_t>(tx.contract.caller.size())
    );
    out.append(
        reinterpret_cast<const char*>(
            tx.contract.caller.data()
        ),
        tx.contract.caller.size()
    );

    WriteString(
        out,
        tx.contract.contractAddress
    );

    WriteU64(
        out,
        tx.contract.deploymentNonce
    );

    WriteU32(
        out,
        static_cast<uint32_t>(tx.contract.bytecode.size())
    );
    out.append(
        reinterpret_cast<const char*>(
            tx.contract.bytecode.data()
        ),
        tx.contract.bytecode.size()
    );

    WriteU32(
        out,
        static_cast<uint32_t>(tx.contract.input.size())
    );
    out.append(
        reinterpret_cast<const char*>(
            tx.contract.input.data()
        ),
        tx.contract.input.size()
    );

    WriteU64(
        out,
        tx.contract.gasLimit
    );

    WriteString(
        out,
        tx.txid
    );

    WriteString(
        out,
        tx.coinbaseData
    );

    WriteU32(
        out,
        static_cast<uint32_t>(tx.vin.size())
    );

    for (const auto& in : tx.vin)
    {
        WriteString(
            out,
            in.previousTx
        );

        WriteU32(
            out,
            in.outputIndex
        );

        WriteU32(
            out,
            static_cast<uint32_t>(
                in.signature.size()
            )
        );

        out.append(
            reinterpret_cast<const char*>(
                in.signature.data()
            ),
            in.signature.size()
        );

        WriteU32(
            out,
            static_cast<uint32_t>(
                in.publicKey.size()
            )
        );

        out.append(
            reinterpret_cast<const char*>(
                in.publicKey.data()
            ),
            in.publicKey.size()
        );
    }

    WriteU32(
        out,
        static_cast<uint32_t>(tx.vout.size())
    );

    for (const auto& o : tx.vout)
    {
        WriteString(
            out,
            o.address
        );

        WriteU64(
            out,
            o.amount
        );

        out.push_back(
            o.spent ? '\1' : '\0'
        );
    }

    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.signature.size()
        )
    );

    out.append(
        reinterpret_cast<const char*>(
            tx.signature.data()
        ),
        tx.signature.size()
    );

    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.publicKey.size()
        )
    );

    out.append(
        reinterpret_cast<const char*>(
            tx.publicKey.data()
        ),
        tx.publicKey.size()
    );
}

// ============================================================
// CANONICAL BLOCK SERIALIZATION
// ============================================================

std::string SerializeBerylBlock(
    const BerylBlock& block
)
{
    std::string out;

    WriteI32(
        out,
        block.header.version
    );

    WriteString(
        out,
        block.header.previousHash
    );

    WriteString(
        out,
        block.header.merkleRoot
    );

    WriteString(
        out,
        block.header.utxoRoot
    );

    WriteString(
        out,
        block.header.contractRoot
    );

    WriteU64(
        out,
        block.header.timestamp
    );

    WriteU32(
        out,
        block.header.nonce
    );

    WriteU64(
        out,
        block.header.difficulty
    );

    WriteI32(
        out,
        block.header.height
    );

    WriteU64(
        out,
        block.reward
    );

    WriteString(
        out,
        block.hash
    );

    WriteU32(
        out,
        static_cast<uint32_t>(
            block.transactions.size()
        )
    );

    for (const auto& tx :
         block.transactions)
    {
        SerializeTransaction(
            out,
            tx
        );
    }

    return out;
}

// ============================================================
// BLOCK HASH
// ============================================================

void UpdateBlockHash(
    BerylBlock& block
)
{
    block.hash =
        GetBlockHash(
            block.header
        );
}

namespace
{

bool ReadU32(
    const std::string& data,
    size_t& pos,
    uint32_t& value
)
{
    if (pos + 4 > data.size())
        return false;

    value =
        static_cast<uint32_t>(
            static_cast<unsigned char>(data[pos])
        )
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(data[pos + 1])
        ) << 8)
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(data[pos + 2])
        ) << 16)
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(data[pos + 3])
        ) << 24);

    pos += 4;
    return true;
}

bool ReadI32(
    const std::string& data,
    size_t& pos,
    int32_t& value
)
{
    uint32_t raw = 0;

    if (!ReadU32(data, pos, raw))
        return false;

    value =
        static_cast<int32_t>(raw);

    return true;
}

bool ReadU64(
    const std::string& data,
    size_t& pos,
    uint64_t& value
)
{
    if (pos + 8 > data.size())
        return false;

    value = 0;

    for (int i = 0; i < 8; ++i)
    {
        value |=
            static_cast<uint64_t>(
                static_cast<unsigned char>(
                    data[pos + i]
                )
            ) << (8 * i);
    }

    pos += 8;
    return true;
}

bool ReadBytes(
    const std::string& data,
    size_t& pos,
    std::string& value
)
{
    uint32_t size = 0;

    if (!ReadU32(data, pos, size))
        return false;

    if (size > data.size() - pos)
        return false;

    value.assign(
        data.data() + pos,
        size
    );

    pos += size;
    return true;
}

bool ReadVector(
    const std::string& data,
    size_t& pos,
    std::vector<unsigned char>& value
)
{
    uint32_t size = 0;

    if (!ReadU32(data, pos, size))
        return false;

    if (size > data.size() - pos)
        return false;

    value.assign(
        reinterpret_cast<const unsigned char*>(
            data.data() + pos
        ),
        reinterpret_cast<const unsigned char*>(
            data.data() + pos + size
        )
    );

    pos += size;
    return true;
}

bool ReadBytes(
    const std::string& data,
    size_t& pos,
    std::vector<unsigned char>& value
)
{
    uint32_t size = 0;

    if (!ReadU32(data, pos, size))
        return false;

    if (size > 16 * 1024 * 1024)
        return false;

    if (pos > data.size() || size > data.size() - pos)
        return false;

    value.assign(
        reinterpret_cast<const unsigned char*>(data.data() + pos),
        reinterpret_cast<const unsigned char*>(data.data() + pos + size)
    );

    pos += size;
    return true;
}


bool ReadTransaction(
    const std::string& data,
    size_t& pos,
    BerylTransaction& tx
)
{
    tx = BerylTransaction{};

    // Native transaction type.
    uint32_t rawType = 0;
    if (!ReadU32(data, pos, rawType))
        return false;

    if (rawType > static_cast<uint32_t>(
            TransactionType::CONTRACT_CALL))
        return false;

    tx.type = static_cast<TransactionType>(rawType);

    // Contract payload type.
    uint32_t rawContractType = 0;
    if (!ReadU32(data, pos, rawContractType))
        return false;

    if (rawContractType > static_cast<uint32_t>(
            TransactionType::CONTRACT_CALL))
        return false;

    tx.contract.type =
        static_cast<TransactionType>(rawContractType);

    if (!ReadBytes(data, pos, tx.contract.caller))
        return false;

    if (!ReadBytes(data, pos, tx.contract.contractAddress))
        return false;

    if (!ReadU64(
            data,
            pos,
            tx.contract.deploymentNonce))
        return false;

    if (!ReadBytes(data, pos, tx.contract.bytecode))
        return false;

    if (!ReadBytes(data, pos, tx.contract.input))
        return false;

    if (!ReadU64(
            data,
            pos,
            tx.contract.gasLimit))
        return false;

    if (!ReadBytes(data, pos, tx.txid))
        return false;

    if (!ReadBytes(data, pos, tx.coinbaseData))
        return false;

    uint32_t inputCount = 0;

    if (!ReadU32(data, pos, inputCount))
        return false;

    if (inputCount > 1000000)
        return false;

    tx.vin.reserve(inputCount);

    for (uint32_t i = 0; i < inputCount; ++i)
    {
        TxInput in{};

        if (!ReadBytes(
                data,
                pos,
                in.previousTx))
            return false;

        if (!ReadU32(
                data,
                pos,
                in.outputIndex))
            return false;

        if (!ReadBytes(
                data,
                pos,
                in.signature))
            return false;

        if (!ReadBytes(
                data,
                pos,
                in.publicKey))
            return false;

        tx.vin.push_back(
            std::move(in)
        );
    }

    uint32_t outputCount = 0;

    if (!ReadU32(
            data,
            pos,
            outputCount))
        return false;

    if (outputCount > 1000000)
        return false;

    tx.vout.reserve(outputCount);

    for (uint32_t i = 0; i < outputCount; ++i)
    {
        TxOutput out{};

        if (!ReadBytes(
                data,
                pos,
                out.address))
            return false;

        if (!ReadU64(
                data,
                pos,
                out.amount))
            return false;

        if (pos >= data.size())
            return false;

        out.spent =
            data[pos++] != '\0';

        tx.vout.push_back(
            std::move(out)
        );
    }

    if (!ReadVector(
            data,
            pos,
            tx.signature))
        return false;

    if (!ReadVector(
            data,
            pos,
            tx.publicKey))
        return false;

    return true;
}

}

bool DeserializeBerylBlock(
    const std::string& data,
    BerylBlock& block
)
{
    size_t pos = 0;

    block = BerylBlock{};

    int32_t version = 0;

    if (!ReadI32(
            data,
            pos,
            version))
        return false;

    block.header.version =
        version;

    if (!ReadBytes(
            data,
            pos,
            block.header.previousHash))
        return false;

    if (!ReadBytes(
            data,
            pos,
            block.header.merkleRoot))
        return false;

    if (!ReadBytes(
            data,
            pos,
            block.header.utxoRoot))
        return false;

    if (!ReadBytes(
            data,
            pos,
            block.header.contractRoot))
        return false;

    if (!ReadU64(
            data,
            pos,
            block.header.timestamp))
        return false;

    if (!ReadU32(
            data,
            pos,
            block.header.nonce))
        return false;

    uint64_t difficulty = 0;

    if (!ReadU64(
            data,
            pos,
            difficulty))
        return false;

    block.header.difficulty =
        difficulty;

    int32_t height = 0;

    if (!ReadI32(
            data,
            pos,
            height))
        return false;

    block.header.height =
        height;

    if (!ReadU64(
            data,
            pos,
            block.reward))
        return false;

    if (!ReadBytes(
            data,
            pos,
            block.hash))
        return false;

    uint32_t txCount = 0;

    if (!ReadU32(
            data,
            pos,
            txCount))
        return false;

    if (txCount > 1000000)
        return false;

    block.transactions.reserve(txCount);

    for (uint32_t i = 0; i < txCount; ++i)
    {
        BerylTransaction tx;

        if (!ReadTransaction(
                data,
                pos,
                tx))
            return false;

        block.transactions.push_back(
            std::move(tx)
        );
    }

    return pos == data.size();
}
