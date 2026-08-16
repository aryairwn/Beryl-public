// © Arya Irawan — 10 August 2026

#include "p2p.h"
#include <atomic>

#include "blockhash.h"
#include "blockvalidation.h"
#include "transaction.h"
#include "storage.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static bool SendBlockToPeer(
    int socketFd,
    const BerylBlock& block
);

namespace
{

static constexpr uint16_t DEFAULT_P2P_PORT = 18445;

static uint16_t GetP2PPort()
{
    const char* env = std::getenv("BERYL_P2P_PORT");

    if (env == nullptr || *env == '\0')
        return DEFAULT_P2P_PORT;

    try
    {
        unsigned long value = std::stoul(env);

        if (value == 0 || value > 65535)
            return DEFAULT_P2P_PORT;

        return static_cast<uint16_t>(value);
    }
    catch (...)
    {
        return DEFAULT_P2P_PORT;
    }
}
static constexpr uint32_t MAX_MESSAGE_SIZE = 64U * 1024U * 1024U;
static constexpr uint32_t MAX_BLOCKS_PER_RESPONSE = 1000;

static std::mutex g_chainMutex;
static std::atomic<bool> g_p2pReady{false};

// ============================================================
// BLOCK RELAY FORWARD DECLARATION
// ============================================================

// ============================================================
// ACTIVE P2P PEERS
// ============================================================
// Socket peer yang sedang aktif disimpan agar transaksi baru
// dapat langsung di-relay ke semua peer.
// ============================================================

static std::mutex g_peerMutex;
static std::vector<int> g_peerSockets;

// ============================================================
// BLOCK RELAY FORWARD DECLARATIONS
// ============================================================


static void RegisterPeerSocket(
    int socketFd
)
{
    std::lock_guard<std::mutex> lock(g_peerMutex);

    if (std::find(
            g_peerSockets.begin(),
            g_peerSockets.end(),
            socketFd) == g_peerSockets.end())
    {
        g_peerSockets.push_back(socketFd);

        std::cout
            << "P2P PEER REGISTERED: socket="
            << socketFd
            << "\n";
    }
}

static void UnregisterPeerSocket(
    int socketFd
)
{
    std::lock_guard<std::mutex> lock(g_peerMutex);

    auto it = std::remove(
        g_peerSockets.begin(),
        g_peerSockets.end(),
        socketFd
    );

    if (it != g_peerSockets.end())
    {
        g_peerSockets.erase(
            it,
            g_peerSockets.end()
        );

        std::cout
            << "P2P PEER UNREGISTERED: socket="
            << socketFd
            << "\n";
    }
}


/*
 * Wire format:
 *
 *   4 bytes  magic
 *   12 bytes command
 *   4 bytes payload size
 *   N bytes payload
 *
 * Semua integer wire memakai little-endian.
 */

static constexpr char P2P_MAGIC[4] = {'B', 'E', 'R', '1'};

static void WriteU32(
    std::string& out,
    uint32_t value
)
{
    out.push_back(static_cast<char>(value & 0xff));
    out.push_back(static_cast<char>((value >> 8) & 0xff));
    out.push_back(static_cast<char>((value >> 16) & 0xff));
    out.push_back(static_cast<char>((value >> 24) & 0xff));
}

static void WriteU64(
    std::string& out,
    uint64_t value
)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(
            static_cast<char>((value >> (i * 8)) & 0xff)
        );
    }
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

static bool ReadU32(
    const std::string& in,
    size_t& pos,
    uint32_t& value
)
{
    if (pos + 4 > in.size())
        return false;

    value =
        static_cast<uint32_t>(
            static_cast<unsigned char>(in[pos])
        )
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(in[pos + 1])
        ) << 8)
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(in[pos + 2])
        ) << 16)
        |
        (static_cast<uint32_t>(
            static_cast<unsigned char>(in[pos + 3])
        ) << 24);

    pos += 4;
    return true;
}

static bool ReadU64(
    const std::string& in,
    size_t& pos,
    uint64_t& value
)
{
    if (pos + 8 > in.size())
        return false;

    value = 0;

    for (int i = 0; i < 8; ++i)
    {
        value |=
            static_cast<uint64_t>(
                static_cast<unsigned char>(in[pos + i])
            ) << (i * 8);
    }

    pos += 8;
    return true;
}

static bool ReadI32(
    const std::string& in,
    size_t& pos,
    int32_t& value
)
{
    uint32_t tmp = 0;

    if (!ReadU32(in, pos, tmp))
        return false;

    value = static_cast<int32_t>(tmp);
    return true;
}

static void WriteString(
    std::string& out,
    const std::string& value
)
{
    WriteU32(
        out,
        static_cast<uint32_t>(value.size())
    );

    out.append(value);
}

static bool ReadString(
    const std::string& in,
    size_t& pos,
    std::string& value
)
{
    uint32_t size = 0;

    if (!ReadU32(in, pos, size))
        return false;

    if (size > MAX_MESSAGE_SIZE)
        return false;

    if (pos + size > in.size())
        return false;

    value.assign(
        in.data() + pos,
        size
    );

    pos += size;
    return true;
}

static bool SendAll(
    int socketFd,
    const std::string& data
)
{
    size_t sent = 0;

    while (sent < data.size())
    {
        ssize_t n = send(
            socketFd,
            data.data() + sent,
            data.size() - sent,
            0
        );

        if (n <= 0)
            return false;

        sent += static_cast<size_t>(n);
    }

    return true;
}

static bool RecvAll(
    int socketFd,
    void* buffer,
    size_t size
)
{
    size_t received = 0;

    while (received < size)
    {
        ssize_t n = recv(
            socketFd,
            static_cast<char*>(buffer) + received,
            size - received,
            0
        );

        if (n <= 0)
            return false;

        received += static_cast<size_t>(n);
    }

    return true;
}

static bool SendMessage(
    int socketFd,
    const std::string& command,
    const std::string& payload
)
{
    if (command.size() > 12)
        return false;

    if (payload.size() > MAX_MESSAGE_SIZE)
        return false;

    std::string header;

    header.append(
        P2P_MAGIC,
        sizeof(P2P_MAGIC)
    );

    char commandField[12];
    std::memset(
        commandField,
        0,
        sizeof(commandField)
    );

    std::memcpy(
        commandField,
        command.data(),
        command.size()
    );

    header.append(
        commandField,
        sizeof(commandField)
    );

    WriteU32(
        header,
        static_cast<uint32_t>(payload.size())
    );

    if (!SendAll(socketFd, header))
        return false;

    if (!payload.empty())
    {
        if (!SendAll(socketFd, payload))
            return false;
    }

    return true;
}

static bool ReceiveMessage(
    int socketFd,
    std::string& command,
    std::string& payload
)
{
    char magic[4];

    if (!RecvAll(
            socketFd,
            magic,
            sizeof(magic)))
    {
        return false;
    }

    if (std::memcmp(
            magic,
            P2P_MAGIC,
            sizeof(P2P_MAGIC)) != 0)
    {
        return false;
    }

    char commandField[12];

    if (!RecvAll(
            socketFd,
            commandField,
            sizeof(commandField)))
    {
        return false;
    }

    command.assign(
        commandField,
        strnlen(commandField, sizeof(commandField))
    );

    char sizeBytes[4];

    if (!RecvAll(
            socketFd,
            sizeBytes,
            sizeof(sizeBytes)))
    {
        return false;
    }

    std::string sizeData(
        sizeBytes,
        sizeof(sizeBytes)
    );

    size_t pos = 0;
    uint32_t payloadSize = 0;

    if (!ReadU32(
            sizeData,
            pos,
            payloadSize))
    {
        return false;
    }

    if (payloadSize > MAX_MESSAGE_SIZE)
        return false;

    payload.clear();

    if (payloadSize == 0)
        return true;

    payload.resize(payloadSize);

    return RecvAll(
        socketFd,
        &payload[0],
        payloadSize
    );
}

/*
 * Transaction serialization untuk wire P2P.
 *
 * Kita sengaja membuat format eksplisit, bukan memcpy(struct),
 * sehingga perubahan alignment compiler tidak merusak jaringan.
 */
static void SerializeTransaction(
    std::string& out,
    const BerylTransaction& tx
)
{
    // ========================================================
    // TXID
    // ========================================================
    WriteString(
        out,
        tx.txid
    );

    // ========================================================
    // NATIVE TRANSACTION TYPE
    // ========================================================
    out.push_back(
        static_cast<char>(
            static_cast<uint8_t>(tx.type)
        )
    );

    // ========================================================
    // CONTRACT TYPE
    // ========================================================
    out.push_back(
        static_cast<char>(
            static_cast<uint8_t>(tx.contract.type)
        )
    );

    // ========================================================
    // CONTRACT CALLER
    // ========================================================
    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.contract.caller.size()
        )
    );

    out.append(
        reinterpret_cast<const char*>(
            tx.contract.caller.data()
        ),
        tx.contract.caller.size()
    );

    // ========================================================
    // CONTRACT ADDRESS
    // ========================================================
    WriteString(
        out,
        tx.contract.contractAddress
    );

    // ========================================================
    // DEPLOYMENT NONCE
    // ========================================================
    WriteU64(
        out,
        tx.contract.deploymentNonce
    );

    // ========================================================
    // BYTECODE
    // ========================================================
    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.contract.bytecode.size()
        )
    );

    out.append(
        reinterpret_cast<const char*>(
            tx.contract.bytecode.data()
        ),
        tx.contract.bytecode.size()
    );

    // ========================================================
    // CONTRACT INPUT
    // ========================================================
    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.contract.input.size()
        )
    );

    out.append(
        reinterpret_cast<const char*>(
            tx.contract.input.data()
        ),
        tx.contract.input.size()
    );

    // ========================================================
    // GAS LIMIT
    // ========================================================
    WriteU64(
        out,
        tx.contract.gasLimit
    );

    // ========================================================
    // COINBASE DATA
    // ========================================================
    WriteString(
        out,
        tx.coinbaseData
    );

    // ========================================================
    // INPUTS
    // ========================================================
    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.vin.size()
        )
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

    // ========================================================
    // OUTPUTS
    // ========================================================
    WriteU32(
        out,
        static_cast<uint32_t>(
            tx.vout.size()
        )
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

    // ========================================================
    // TRANSACTION-LEVEL FALCON SIGNATURE
    // ========================================================
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

    // ========================================================
    // TRANSACTION-LEVEL FALCON PUBLIC KEY
    // ========================================================
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

static bool SendTransactionToPeer(
    int socketFd,
    const BerylTransaction& tx
)
{
    std::string payload;
    SerializeTransaction(payload, tx);

    return SendMessage(
        socketFd,
        "tx",
        payload
    );
}

static bool ReadBytes(
    const std::string& in,
    size_t& pos,
    std::vector<unsigned char>& out
)
{
    uint32_t size = 0;

    if (!ReadU32(in, pos, size))
        return false;

    if (size > MAX_MESSAGE_SIZE)
        return false;

    if (pos + size > in.size())
        return false;

    out.assign(
        reinterpret_cast<const unsigned char*>(
            in.data() + pos
        ),
        reinterpret_cast<const unsigned char*>(
            in.data() + pos + size
        )
    );

    pos += size;
    return true;
}


static bool ReadBytes(
    const std::string& in,
    size_t& pos,
    std::string& out
)
{
    uint32_t size = 0;

    if (!ReadU32(in, pos, size))
        return false;

    if (size > MAX_MESSAGE_SIZE)
        return false;

    if (pos + size > in.size())
        return false;

    out.assign(
        in.data() + pos,
        size
    );

    pos += size;
    return true;
}

static bool DeserializeTransaction(
    const std::string& in,
    size_t& pos,
    BerylTransaction& tx
)
{
    tx = BerylTransaction();

    // ========================================================
    // NATIVE TRANSACTION TYPE
    // Canonical serialization uses U32.
    // ========================================================
    uint32_t rawType = 0;

    if (!ReadU32(in, pos, rawType))
        return false;

    if (rawType >
        static_cast<uint32_t>(TransactionType::CONTRACT_CALL))
        return false;

    tx.type =
        static_cast<TransactionType>(rawType);

    // ========================================================
    // CONTRACT TYPE
    // Canonical serialization uses U32.
    // ========================================================
    uint32_t rawContractType = 0;

    if (!ReadU32(in, pos, rawContractType))
        return false;

    if (rawContractType >
        static_cast<uint32_t>(TransactionType::CONTRACT_CALL))
        return false;

    tx.contract.type =
        static_cast<TransactionType>(rawContractType);

    // ========================================================
    // CONTRACT CALLER
    // ========================================================
    if (!ReadBytes(
            in,
            pos,
            tx.contract.caller))
        return false;

    // ========================================================
    // CONTRACT ADDRESS
    // ========================================================
    if (!ReadString(
            in,
            pos,
            tx.contract.contractAddress))
        return false;

    // ========================================================
    // DEPLOYMENT NONCE
    // ========================================================
    if (!ReadU64(
            in,
            pos,
            tx.contract.deploymentNonce))
        return false;

    // ========================================================
    // BYTECODE
    // ========================================================
    if (!ReadBytes(
            in,
            pos,
            tx.contract.bytecode))
        return false;

    // ========================================================
    // CONTRACT INPUT
    // ========================================================
    if (!ReadBytes(
            in,
            pos,
            tx.contract.input))
        return false;

    // ========================================================
    // GAS LIMIT
    // ========================================================
    if (!ReadU64(
            in,
            pos,
            tx.contract.gasLimit))
        return false;

    // ========================================================
    // TXID
    // Canonical serialization places TXID after gasLimit.
    // ========================================================
    if (!ReadString(
            in,
            pos,
            tx.txid))
        return false;

    // ========================================================
    // COINBASE DATA
    // ========================================================
    if (!ReadString(
            in,
            pos,
            tx.coinbaseData))
        return false;

    // ========================================================
    // INPUTS
    // ========================================================
    uint32_t vinCount = 0;

    if (!ReadU32(in, pos, vinCount))
        return false;

    if (vinCount > 100000)
        return false;

    tx.vin.reserve(vinCount);

    for (uint32_t i = 0; i < vinCount; ++i)
    {
        TxInput input;

        if (!ReadString(
                in,
                pos,
                input.previousTx))
            return false;

        if (!ReadU32(
                in,
                pos,
                input.outputIndex))
            return false;

        if (!ReadBytes(
                in,
                pos,
                input.signature))
            return false;

        if (!ReadBytes(
                in,
                pos,
                input.publicKey))
            return false;

        tx.vin.push_back(
            std::move(input)
        );
    }

    // ========================================================
    // OUTPUTS
    // ========================================================
    uint32_t voutCount = 0;

    if (!ReadU32(in, pos, voutCount))
        return false;

    if (voutCount > 100000)
        return false;

    tx.vout.reserve(voutCount);

    for (uint32_t i = 0; i < voutCount; ++i)
    {
        TxOutput output;

        if (!ReadString(
                in,
                pos,
                output.address))
            return false;

        if (!ReadU64(
                in,
                pos,
                output.amount))
            return false;

        if (pos >= in.size())
            return false;

        output.spent =
            in[pos++] != 0;

        tx.vout.push_back(
            std::move(output)
        );
    }

    // ========================================================
    // TRANSACTION-LEVEL FALCON SIGNATURE
    // ========================================================
    if (!ReadBytes(
            in,
            pos,
            tx.signature))
        return false;

    // ========================================================
    // TRANSACTION-LEVEL FALCON PUBLIC KEY
    // ========================================================
    if (!ReadBytes(
            in,
            pos,
            tx.publicKey))
        return false;

    return true;
}

static std::string SerializeBlock(
    const BerylBlock& block
)
{
    return SerializeBerylBlock(block);
}

static bool DeserializeBlock(
    const std::string& in,
    BerylBlock& block
)
{
    size_t pos = 0;

    int32_t version = 0;

    if (!ReadI32(
            in,
            pos,
            version))
        return false;

    block.header.version =
        static_cast<int>(version);

    if (!ReadString(
            in,
            pos,
            block.header.previousHash))
        return false;

    if (!ReadString(
            in,
            pos,
            block.header.merkleRoot))
        return false;

    if (!ReadString(
            in,
            pos,
            block.header.utxoRoot))
        return false;

    if (!ReadString(
            in,
            pos,
            block.header.contractRoot))
        return false;

    if (!ReadU64(
            in,
            pos,
            block.header.timestamp))
        return false;

    if (!ReadU32(
            in,
            pos,
            block.header.nonce))
        return false;

    if (!ReadU64(
            in,
            pos,
            block.header.difficulty))
        return false;

    int32_t height = 0;

    if (!ReadI32(
            in,
            pos,
            height))
        return false;

    block.header.height =
        static_cast<int>(height);

    if (!ReadU64(
            in,
            pos,
            block.reward))
        return false;

    if (!ReadString(
            in,
            pos,
            block.hash))
        return false;

    uint32_t txCount = 0;

    if (!ReadU32(
            in,
            pos,
            txCount))
        return false;

    if (txCount > 100000)
        return false;

    block.transactions.clear();

    for (uint32_t i = 0; i < txCount; ++i)
    {
        BerylTransaction tx;

        if (!DeserializeTransaction(
                in,
                pos,
                tx))
            return false;

        block.transactions.push_back(
            std::move(tx)
        );
    }

    return pos == in.size();
}

static std::string SerializeBlocks(
    const std::vector<BerylBlock>& blocks
)
{
    std::string payload;

    WriteU32(
        payload,
        static_cast<uint32_t>(
            blocks.size()
        )
    );

    for (const auto& block : blocks)
    {
        const std::string serialized =
            SerializeBlock(block);

        WriteU32(
            payload,
            static_cast<uint32_t>(
                serialized.size()
            )
        );

        payload.append(serialized);
    }

    return payload;
}

static bool DeserializeBlocks(
    const std::string& payload,
    std::vector<BerylBlock>& blocks
)
{
    size_t pos = 0;

    uint32_t count = 0;

    if (!ReadU32(
            payload,
            pos,
            count))
        return false;

    if (count > MAX_BLOCKS_PER_RESPONSE)
        return false;

    blocks.clear();

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t size = 0;

        if (!ReadU32(
                payload,
                pos,
                size))
            return false;

        if (size > MAX_MESSAGE_SIZE)
            return false;

        if (pos + size > payload.size())
            return false;

        std::string blockData =
            payload.substr(
                pos,
                size
            );

        pos += size;

        BerylBlock block;

        if (!DeserializeBlock(
                blockData,
                block))
            return false;

        blocks.push_back(
            std::move(block)
        );
    }

    return pos == payload.size();
}

static bool SendVersion(
    int socketFd,
    const BerylChain& chain
)
{
    std::ostringstream out;

    out << "version=1\n";
    out << "height="
        << chain.GetHeight()
        << "\n";

    return SendMessage(
        socketFd,
        "version",
        out.str()
    );
}

static bool ConnectToPeer(
    const std::string& host,
    uint16_t port,
    BerylChain& chain,
    const std::string& blockchainFile
)
{
    std::cout
        << "P2P OUTBOUND: connecting to "
        << host << ":" << port << "\n";

    int socketFd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (socketFd < 0)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: socket failed\n";
        return false;
    }

    sockaddr_in peer{};
    peer.sin_family = AF_INET;
    peer.sin_port = htons(port);

    if (inet_pton(
            AF_INET,
            host.c_str(),
            &peer.sin_addr
        ) != 1)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: invalid IPv4 address\n";

        close(socketFd);
        return false;
    }

    if (connect(
            socketFd,
            reinterpret_cast<sockaddr*>(&peer),
            sizeof(peer)
        ) < 0)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: connect failed to "
            << host << ":" << port << "\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: TCP connected to "
        << host << ":" << port << "\n";

    // --------------------------------------------------------
    // 1. Peer harus mengirim version terlebih dahulu.
    // --------------------------------------------------------

    std::string command;
    std::string payload;

    if (!ReceiveMessage(
            socketFd,
            command,
            payload
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed receiving version\n";

        close(socketFd);
        return false;
    }

    if (command != "version")
    {
        std::cerr
            << "P2P OUTBOUND ERROR: expected version, got "
            << command << "\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: received version\n"
        << payload;

    // --------------------------------------------------------
    // 2. Balas version milik kita.
    // --------------------------------------------------------

    if (!SendVersion(
            socketFd,
            chain
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed sending version\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: sent version\n";

    // --------------------------------------------------------
    // 3. Peer membalas verack.
    // --------------------------------------------------------

    if (!ReceiveMessage(
            socketFd,
            command,
            payload
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed receiving verack\n";

        close(socketFd);
        return false;
    }

    if (command != "verack")
    {
        std::cerr
            << "P2P OUTBOUND ERROR: expected verack, got "
            << command << "\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: received verack\n";

    // --------------------------------------------------------
    // 4. Kirim verack kita.
    // --------------------------------------------------------

    if (!SendMessage(
            socketFd,
            "verack",
            ""
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed sending verack\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: sent verack\n";

    // --------------------------------------------------------
    // 5. Server kita sekarang akan mengirim getheight.
    // --------------------------------------------------------

    if (!ReceiveMessage(
            socketFd,
            command,
            payload
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed receiving post-handshake message\n";

        close(socketFd);
        return false;
    }

    if (command != "getheight")
    {
        std::cerr
            << "P2P OUTBOUND: expected getheight, got "
            << command
            << "\n";

        close(socketFd);
        return false;
    }

    // Balas height kita.
    if (!SendMessage(
            socketFd,
            "height",
            std::to_string(chain.GetHeight())
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed sending height\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: handshake OK\n"
        << "P2P OUTBOUND: local height="
        << chain.GetHeight()
        << "\n";

    // --------------------------------------------------------
    // 6. Tunggu height dari peer.
    // --------------------------------------------------------

    if (!ReceiveMessage(
            socketFd,
            command,
            payload
        ))
    {
        std::cerr
            << "P2P OUTBOUND ERROR: failed receiving peer height\n";

        close(socketFd);
        return false;
    }

    if (command != "height")
    {
        std::cerr
            << "P2P OUTBOUND ERROR: expected height, got "
            << command
            << "\n";

        close(socketFd);
        return false;
    }

    int64_t peerHeight = -1;

    try
    {
        peerHeight =
            std::stoll(payload);
    }
    catch (...)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: invalid peer height\n";

        close(socketFd);
        return false;
    }

    if (peerHeight < 0)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: negative peer height\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P OUTBOUND: peer height="
        << peerHeight
        << "\n";

    // --------------------------------------------------------
    // 7. Sinkronisasi block.
    //
    // Kita hanya meminta block setelah height lokal.
    // Tidak ada reorg pada tahap ini.
    // --------------------------------------------------------

    while (chain.GetHeight() < peerHeight)
    {
        const uint32_t startHeight =
            static_cast<uint32_t>(chain.GetHeight());

        std::string request;
        WriteU32(request, startHeight);

        std::cout
            << "P2P SYNC: requesting blocks from height "
            << startHeight + 1
            << "\n";

        if (!SendMessage(
                socketFd,
                "getblocks",
                request
            ))
        {
            std::cerr
                << "P2P SYNC ERROR: failed sending getblocks\n";

            close(socketFd);
            return false;
        }

        if (!ReceiveMessage(
                socketFd,
                command,
                payload
            ))
        {
            std::cerr
                << "P2P SYNC ERROR: failed receiving blocks\n";

            close(socketFd);
            return false;
        }

        if (command != "blocks")
        {
            std::cerr
                << "P2P SYNC ERROR: expected blocks, got "
                << command
                << "\n";

            close(socketFd);
            return false;
        }

        std::vector<BerylBlock> blocks;

        if (!DeserializeBlocks(
                payload,
                blocks
            ))
        {
            std::cerr
                << "P2P SYNC ERROR: invalid blocks payload\n";

            close(socketFd);
            return false;
        }

        if (blocks.empty())
        {
            std::cerr
                << "P2P SYNC ERROR: peer returned zero blocks\n";

            close(socketFd);
            return false;
        }

        std::cout
            << "P2P SYNC: received "
            << blocks.size()
            << " block(s)\n";

        for (const auto& block : blocks)
        {
            // Jangan menerima block yang melebihi height
            // yang tadi diumumkan peer.
            if (block.header.height > peerHeight)
            {
                std::cout
                    << "P2P SYNC: peer advanced during sync, "
                    << "stopping at advertised height="
                    << peerHeight
                    << "\\n";
                break;
            }

            std::lock_guard<std::mutex> lock(g_chainMutex);

            if (!chain.AddBlock(block))
            {
                std::cerr
                    << "P2P SYNC ERROR: block rejected"
                    << " HEIGHT="
                    << block.header.height
                    << "\n";

                close(socketFd);
                return false;
            }

            std::cout
                << "P2P SYNC: block accepted"
                << " HEIGHT="
                << block.header.height
                << " HASH="
                << block.hash
                << "\n";
        }
    }

    std::cout
        << "P2P SYNC COMPLETE: local height="
        << chain.GetHeight()
        << "\n";

    // --------------------------------------------------------
    // PERSIST INITIAL P2P SYNCHRONIZATION
    // --------------------------------------------------------

    if (!SaveBlockchain(
            chain,
            blockchainFile))
    {
        std::cerr
            << "P2P SYNC ERROR: failed saving blockchain"
            << " FILE="
            << blockchainFile
            << "\n";

        close(socketFd);
        return false;
    }

    std::cout
        << "P2P SYNC: blockchain saved"
        << " FILE="
        << blockchainFile
        << " HEIGHT="
        << chain.GetHeight()
        << "\n";

    g_p2pReady.store(
        true,
        std::memory_order_release
    );

    std::cout
        << "P2P READY: initial synchronization complete\n";

    /*
     * Jangan tutup socket.
     * Koneksi outbound dipertahankan untuk relay transaksi
     * dan block.
     */
    RegisterPeerSocket(socketFd);

    std::cout
        << "P2P OUTBOUND: persistent peer connection established\n";

    /*
     * Untuk tahap ini kita menjaga koneksi tetap hidup.
     * Pesan ping/pong dan tx akan diproses.
     */
    while (true)
    {
        std::string liveCommand;
        std::string livePayload;

        if (!ReceiveMessage(
                socketFd,
                liveCommand,
                livePayload))
        {
            break;
        }

        if (liveCommand == "ping")
        {
            if (!SendMessage(
                    socketFd,
                    "pong",
                    livePayload))
            {
                break;
            }
        }
        else if (liveCommand == "tx")
        {
            BerylTransaction tx;
            size_t txPos = 0;

            if (!DeserializeTransaction(
                    livePayload,
                    txPos,
                    tx))
            {
                std::cerr
                    << "P2P OUTBOUND: invalid tx payload\n";
                break;
            }

            /*
             * ConnectToPeer() hanya mempunyai chain.
             * Mempool belum tersedia di fungsi ini, sehingga
             * transaksi inbound dari persistent outbound peer
             * akan ditangani pada tahap peer-session berikutnya.
             */
        }
        else if (liveCommand == "block")
        {
            BerylBlock block;

            if (!DeserializeBlock(
                    livePayload,
                    block))
            {
                std::cerr
                    << "P2P OUTBOUND: invalid block payload\n";
                break;
            }

            std::lock_guard<std::mutex> lock(g_chainMutex);

            if (!chain.AddBlock(block))
            {
                std::cerr
                    << "P2P OUTBOUND: received block rejected"
                    << " HEIGHT="
                    << block.header.height
                    << "\n";
            }
            else
            {
                std::cout
                    << "P2P OUTBOUND: received block accepted"
                    << " HEIGHT="
                    << block.header.height
                    << "\n";
            }
        }
        else
        {
            std::cout
                << "P2P OUTBOUND: received "
                << liveCommand
                << "\n";
        }
    }

    UnregisterPeerSocket(socketFd);
    close(socketFd);

    std::cout
        << "P2P OUTBOUND: persistent peer disconnected\n";

    return true;
}

static void HandlePeer(
    int socketFd,
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool
)
{
    std::cout
        << "P2P peer connected\n";

    if (!SendVersion(
            socketFd,
            chain))
    {
        close(socketFd);
        return;
    }

    bool running = true;

    while (running)
    {
        std::string command;
        std::string payload;

        if (!ReceiveMessage(
                socketFd,
                command,
                payload))
        {
            break;
        }

        if (command == "version")
        {
            SendMessage(
                socketFd,
                "verack",
                ""
            );
        }
        else if (command == "verack")
        {
            SendMessage(
                socketFd,
                "getheight",
                ""
            );
        }
        else if (command == "ping")
        {
            SendMessage(
                socketFd,
                "pong",
                payload
            );
        }
        else if (command == "getheight")
        {
            std::string height =
                std::to_string(
                    chain.GetHeight()
                );

            SendMessage(
                socketFd,
                "height",
                height
            );

            /*
             * Setelah handshake selesai, socket ini
             * siap digunakan untuk relay.
             */
            RegisterPeerSocket(socketFd);
        }
        else if (command == "height")
        {
            // Peer mengirim height setelah handshake.
            std::cout
                << "P2P: received peer height="
                << payload
                << "\n";

            // Balas dengan height lokal agar peer
            // dapat melanjutkan proses sinkronisasi.
            if (!SendMessage(
                    socketFd,
                    "height",
                    std::to_string(chain.GetHeight())))
            {
                break;
            }
        }
        else if (command == "getblocks")
        {
            size_t pos = 0;

            uint32_t startHeight = 0;

            if (!ReadU32(
                    payload,
                    pos,
                    startHeight))
            {
                break;
            }

            std::vector<BerylBlock> blocks;

            {
                std::lock_guard<std::mutex>
                    lock(g_chainMutex);

                const auto& chainBlocks =
                    chain.GetBlocks();

                for (
                    size_t i = startHeight;
                    i < chainBlocks.size() &&
                    blocks.size() < MAX_BLOCKS_PER_RESPONSE;
                    ++i
                )
                {
                    blocks.push_back(
                        chainBlocks[i]
                    );
                }
            }

            const std::string response =
                SerializeBlocks(blocks);

            if (!SendMessage(
                    socketFd,
                    "blocks",
                    response))
            {
                break;
            }
        }
        else if (command == "block")
        {
            BerylBlock block;

            if (!DeserializeBlock(
                    payload,
                    block))
            {
                std::cerr
                    << "P2P: invalid block payload\n";
                break;
            }

            std::lock_guard<std::mutex>
                lock(g_chainMutex);

            /*
             * AddBlock() melakukan:
             *
             * 1. height check
             * 2. ValidateBlock()
             * 3. push ke chain
             * 4. ProcessBlock()
             */
            if (chain.AddBlock(block))
            {
                std::cout
                    << "P2P BLOCK ACCEPTED"
                    << " HEIGHT="
                    << block.header.height
                    << " HASH="
                    << block.hash
                    << "\n";

                SendMessage(
                    socketFd,
                    "blockack",
                    std::to_string(
                        block.header.height
                    )
                );

                /*
                 * Block baru sudah resmi masuk chain.
                 *
                 * Relay ke peer lain, tetapi jangan kirim
                 * kembali ke socket asal agar tidak terjadi
                 * loop A -> B -> A -> B.
                 */
                {
                    std::lock_guard<std::mutex>
                        peerLock(g_peerMutex);

                    for (auto it = g_peerSockets.begin();
                         it != g_peerSockets.end();)
                    {
                        if (*it == socketFd)
                        {
                            ++it;
                            continue;
                        }

                        if (!SendBlockToPeer(
                                *it,
                                block))
                        {
                            close(*it);
                            it = g_peerSockets.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
            else
            {
                std::cerr
                    << "P2P BLOCK REJECTED"
                    << " HEIGHT="
                    << block.header.height
                    << "\n";

                SendMessage(
                    socketFd,
                    "blockreject",
                    std::to_string(
                        block.header.height
                    )
                );
            }
        }
        else if (command == "tx")
        {
            // ========================================================
            // RECEIVE TRANSACTION
            // ========================================================

            BerylTransaction tx;
            size_t txPos = 0;

            if (!DeserializeTransaction(
                    payload,
                    txPos,
                    tx) ||
                txPos != payload.size())
            {
                std::cerr
                    << "P2P: invalid transaction payload\n";

                SendMessage(
                    socketFd,
                    "txreject",
                    "deserialize"
                );

                continue;
            }

            // Jangan menerima TX duplikat.
            if (mempool.Contains(tx.txid))
            {
                SendMessage(
                    socketFd,
                    "txack",
                    tx.txid
                );

                continue;
            }

            // Validasi + masukkan ke mempool.
            if (mempool.AddTransaction(
                    utxoManager.GetUTXOSet(),
                    tx,
                    chain.GetHeight()))
            {
                std::cout
                    << "P2P TX ACCEPTED"
                    << " TXID=" << tx.txid
                    << "\n";

                SendMessage(
                    socketFd,
                    "txack",
                    tx.txid
                );

                // Relay ke peer lainnya.
                std::lock_guard<std::mutex> lock(g_peerMutex);

                for (auto it = g_peerSockets.begin();
                     it != g_peerSockets.end();)
                {
                    if (*it == socketFd)
                    {
                        ++it;
                        continue;
                    }

                    if (!SendTransactionToPeer(
                            *it,
                            tx))
                    {
                        close(*it);
                        it = g_peerSockets.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            else
            {
                std::cerr
                    << "P2P TX REJECTED"
                    << " TXID=" << tx.txid
                    << "\n";

                SendMessage(
                    socketFd,
                    "txreject",
                    tx.txid
                );
            }
        }
        else
        {
            std::cerr
                << "P2P: unknown command: "
                << command
                << "\n";

            SendMessage(
                socketFd,
                "reject",
                command
            );
        }
    }

    UnregisterPeerSocket(socketFd);

    close(socketFd);

    std::cout
        << "P2P peer disconnected\n";
}

static void P2PServer(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool
)
{
    const uint16_t p2pPort = GetP2PPort();

    int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverSocket < 0)
    {
        std::cerr
            << "P2P ERROR: socket failed\n";
        return;
    }

    int reuse = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    sockaddr_in server{};

    server.sin_family =
        AF_INET;

    server.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server.sin_port =
        htons(p2pPort);

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(
                &server
            ),
            sizeof(server)) < 0)
    {
        std::cerr
            << "P2P ERROR: bind failed on port "
            << p2pPort
            << "\n";

        close(serverSocket);
        return;
    }

    if (listen(
            serverSocket,
            16) < 0)
    {
        std::cerr
            << "P2P ERROR: listen failed\n";

        close(serverSocket);
        return;
    }

    std::cout
        << "P2P SERVER LISTENING ON 0.0.0.0:"
        << p2pPort
        << "\n";

    while (true)
    {
        sockaddr_in peer{};
        socklen_t peerLength =
            sizeof(peer);

        int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &peer
                ),
                &peerLength
            );

        if (clientSocket < 0)
            continue;

        std::thread(
            HandlePeer,
            clientSocket,
            std::ref(chain),
            std::ref(utxoManager),
            std::ref(mempool)
        ).detach();
    }
}

}

static bool SendBlockToPeer(
    int socketFd,
    const BerylBlock& block
)
{
    const std::string payload =
        SerializeBlock(block);

    return SendMessage(
        socketFd,
        "block",
        payload
    );
}

void BroadcastBlock(
    const BerylBlock& block
)
{
    std::lock_guard<std::mutex> lock(g_peerMutex);

    for (auto it = g_peerSockets.begin();
         it != g_peerSockets.end();)
    {
        if (!SendBlockToPeer(
                *it,
                block))
        {
            close(*it);
            it = g_peerSockets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void BroadcastTransaction(
    const BerylTransaction& tx
)
{
    std::lock_guard<std::mutex> lock(g_peerMutex);

    for (auto it = g_peerSockets.begin();
         it != g_peerSockets.end();)
    {
        if (!SendTransactionToPeer(*it, tx))
        {
            close(*it);
            it = g_peerSockets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

int GetActivePeerCount()
{
    std::lock_guard<std::mutex> lock(g_peerMutex);
    return static_cast<int>(g_peerSockets.size());
}

bool IsP2PReady()
{
    return g_p2pReady.load(
        std::memory_order_acquire
    );
}

void StartP2P(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool,
    const std::string& blockchainFile
)
{
    // --------------------------------------------------------
    // P2P inbound server.
    // --------------------------------------------------------

    std::thread(
        P2PServer,
        std::ref(chain),
        std::ref(utxoManager),
        std::ref(mempool)
    ).detach();

    // --------------------------------------------------------
    // Optional outbound peer.
    //
    // Format:
    //   BERYL_PEER=127.0.0.1:18445
    //
    // Kalau environment variable tidak ada, node hanya
    // menjalankan inbound P2P seperti sebelumnya.
    // --------------------------------------------------------

    const char* peerEnv =
        std::getenv("BERYL_PEER");

    if (peerEnv == nullptr || *peerEnv == '\0')
    {
        g_p2pReady.store(
            true,
            std::memory_order_release
        );

        std::cout
            << "P2P READY: no outbound peer configured\n";

        return;
    }

    std::string peer = peerEnv;

    const size_t colon =
        peer.rfind(':');

    if (colon == std::string::npos ||
        colon == 0 ||
        colon + 1 >= peer.size())
    {
        std::cerr
            << "P2P OUTBOUND ERROR: "
            << "BERYL_PEER harus IP:PORT\n";

        return;
    }

    const std::string host =
        peer.substr(0, colon);

    const std::string portText =
        peer.substr(colon + 1);

    uint64_t portValue = 0;

    try
    {
        portValue =
            std::stoull(portText);
    }
    catch (...)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: invalid port\n";

        return;
    }

    if (portValue == 0 || portValue > 65535)
    {
        std::cerr
            << "P2P OUTBOUND ERROR: port out of range\n";

        return;
    }

    const uint16_t port =
        static_cast<uint16_t>(portValue);

    std::thread(
        ConnectToPeer,
        host,
        port,
        std::ref(chain),
        std::cref(blockchainFile)
    ).detach();
}
