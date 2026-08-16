// © Arya Irawan — 10 August 2026

#include "rpc.h"
#include "blocktemplate.h"
#include "transaction.h"
#include "p2p.h"
#include "consensus.h"
#include "blockvalidation.h"
#include "storage.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <cstdlib>

static constexpr int DEFAULT_RPC_PORT = 18444;

static int GetRPCPort()
{
    const char* env = std::getenv("BERYL_RPC_PORT");

    if (env == nullptr || *env == '\0')
        return DEFAULT_RPC_PORT;

    try
    {
        long value = std::stol(env);

        if (value <= 0 || value > 65535)
            return DEFAULT_RPC_PORT;

        return static_cast<int>(value);
    }
    catch (...)
    {
        return DEFAULT_RPC_PORT;
    }
}

static bool IsValidBerylAddress(
    const std::string& address
)
{
    if (address.size() != 43)
        return false;

    if (address.compare(0, 3, "ber") != 0)
        return false;

    for (size_t i = 3; i < address.size(); ++i)
    {
        const char c = address[i];

        const bool hex =
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F');

        if (!hex)
            return false;
    }

    return true;
}

static std::string FormatBER(uint64_t amount)
{
    std::ostringstream ss;

    ss << std::fixed
       << std::setprecision(8)
       << (static_cast<double>(amount) / 100000000.0)
       << " BER";

    return ss.str();
}


// ============================================================
// HEX ENCODING
// ============================================================

static std::string BytesToHex(
    const std::string& data
)
{
    static const char* hex =
        "0123456789abcdef";

    std::string out;

    out.reserve(data.size() * 2);

    for (unsigned char c : data)
    {
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 0x0f]);
    }

    return out;
}

static int HexValue(
    char c
)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

static bool HexToBytes(
    const std::string& hex,
    std::string& data
)
{
    if (hex.size() % 2 != 0)
        return false;

    data.clear();
    data.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        const int high =
            HexValue(hex[i]);

        const int low =
            HexValue(hex[i + 1]);

        if (high < 0 || low < 0)
            return false;

        data.push_back(
            static_cast<char>(
                (high << 4) | low
            )
        );
    }

    return true;
}


// ============================================================
// FIND TRANSACTION IN BLOCKCHAIN
// ============================================================
// Mencari transaksi berdasarkan TXID di seluruh blockchain.
//
// Fee TIDAK dihitung dari UTXO aktif, karena input transaksi
// yang sudah masuk block kemungkinan sudah di-Spend().
//
// Untuk transaksi confirmed, nilai input dicari kembali dari
// output transaksi sebelumnya di blockchain.
// ============================================================

static bool FindHistoricalOutput(
    const BerylChain& chain,
    const std::string& txid,
    uint32_t index,
    TxOutput& result
)
{
    const auto& blocks = chain.GetBlocks();

    for (const auto& block : blocks)
    {
        for (const auto& tx : block.transactions)
        {
            if (tx.txid != txid)
                continue;

            if (index >= tx.vout.size())
                return false;

            result = tx.vout[index];
            return true;
        }
    }

    return false;
}

static bool FindTransactionInChain(
    const BerylChain& chain,
    const std::string& txid,
    BerylTransaction& result,
    int& blockHeight
)
{
    const auto& blocks = chain.GetBlocks();

    for (const auto& block : blocks)
    {
        for (const auto& tx : block.transactions)
        {
            if (tx.txid == txid)
            {
                result = tx;
                blockHeight = block.header.height;
                return true;
            }
        }
    }

    return false;
}

static uint64_t CalculateHistoricalTransactionFee(
    const BerylChain& chain,
    const BerylTransaction& tx
)
{
    // Coinbase tidak mempunyai input dan tidak membayar fee.
    if (tx.vin.empty())
        return 0;

    uint64_t totalInput = 0;
    uint64_t totalOutput = 0;

    // --------------------------------------------------------
    // Cari seluruh input dari transaksi sebelumnya.
    // --------------------------------------------------------
    for (const auto& input : tx.vin)
    {
        TxOutput previousOutput;

        if (!FindHistoricalOutput(
                chain,
                input.previousTx,
                input.outputIndex,
                previousOutput))
        {
            return 0;
        }

        if (UINT64_MAX - totalInput < previousOutput.amount)
            return 0;

        totalInput += previousOutput.amount;
    }

    // --------------------------------------------------------
    // Hitung seluruh output transaksi.
    // --------------------------------------------------------
    for (const auto& output : tx.vout)
    {
        if (UINT64_MAX - totalOutput < output.amount)
            return 0;

        totalOutput += output.amount;
    }

    if (totalInput < totalOutput)
        return 0;

    return totalInput - totalOutput;
}

static std::string ProcessRPC(
    const std::string& command,
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool,
    const std::string& blockchainFile
)
{
    if (command == "help")
    {
        return
            "Beryl Full Node RPC commands:\n"
            "  help\n"
            "  getblockcount\n"
            "  getblock <height>\n"
            "  getdifficulty\n"
            "  getaddressbalance <address>\n"
            "  getaddressutxos <address>\n"
            "  gettransaction <txid>\n"
            "  submittransaction <txhex>\n"
            "  getblocktemplate <miner-address>\n"
            "  submitblock <blockhex>\n"
            "  getmininginfo\n";
    }

    // ============================================================
    // GET BLOCK TEMPLATE
    //
    // Format:
    //   getblocktemplate <miner-address>
    //
    // Node membuat candidate block tanpa melakukan PoW.
    // ============================================================

    if (command.rfind("getblocktemplate ", 0) == 0)
    {
        const std::string minerAddress =
            command.substr(
                std::string("getblocktemplate ").size()
            );

        if (minerAddress.empty())
            return "ERROR: usage: getblocktemplate <miner-address>\n";

        if (!IsValidBerylAddress(minerAddress))
        {
            return "ERROR: invalid Beryl mining address\n";
        }

        BerylBlock block;

        if (!CreateBlockTemplate(
                block,
                chain,
                mempool,
                minerAddress))
        {
            return "ERROR: failed to create block template\n";
        }

        const std::string serialized =
            SerializeBerylBlock(block);

        const std::string blockHex =
            BytesToHex(serialized);

        std::ostringstream out;

        out << "height="
            << block.header.height
            << "\n";

        out << "previoushash="
            << block.header.previousHash
            << "\n";

        out << "difficulty="
            << block.header.difficulty
            << "\n";

        out << "timestamp="
            << block.header.timestamp
            << "\n";

        out << "reward="
            << FormatBER(block.reward)
            << "\n";

        out << "transactions="
            << block.transactions.size()
            << "\n";

        out << "blockhex="
            << blockHex
            << "\n";

        return out.str();
    }

    if (command == "getblockcount")
    {
        return std::to_string(chain.GetHeight()) + "\n";
    }

    if (command.rfind("getblock ", 0) == 0)
    {
        const std::string heightText =
            command.substr(9);

        try
        {
            const int height =
                std::stoi(heightText);

            if (height < 1 ||
                height > chain.GetHeight())
            {
                return "ERROR: block height tidak ditemukan\n";
            }

            // RPC memakai height 1-based.
            // BerylChain::GetBlock() memakai index 0-based.
            const BerylBlock& block =
                chain.GetBlock(height - 1);

            std::ostringstream out;

            out << "height=" << block.header.height << "\n";
            out << "hash=" << block.hash << "\n";
            out << "previousHash="
                << block.header.previousHash << "\n";
            out << "version="
                << block.header.version << "\n";
            out << "timestamp="
                << block.header.timestamp << "\n";
            out << "nonce="
                << block.header.nonce << "\n";
            out << "difficulty="
                << block.header.difficulty << "\n";
            out << "reward="
                << FormatBER(block.reward) << "\n";
            out << "merkleRoot="
                << block.header.merkleRoot << "\n";
            out << "utxoRoot="
                << block.header.utxoRoot << "\n";
            out << "transactions="
                << block.transactions.size() << "\n";

            for (size_t i = 0;
                 i < block.transactions.size();
                 ++i)
            {
                const auto& tx =
                    block.transactions[i];

                out << "tx[" << i << "].txid="
                    << tx.txid << "\n";

                out << "tx[" << i << "].inputs="
                    << tx.vin.size() << "\n";

                out << "tx[" << i << "].outputs="
                    << tx.vout.size() << "\n";

                for (size_t j = 0;
                     j < tx.vout.size();
                     ++j)
                {
                    out << "tx[" << i
                        << "].output[" << j
                        << "].address="
                        << tx.vout[j].address
                        << "\n";

                    out << "tx[" << i
                        << "].output[" << j
                        << "].amount="
                        << FormatBER(
                            tx.vout[j].amount
                        )
                        << "\n";
                }
            }

            return out.str();
        }
        catch (...)
        {
            return "ERROR: block height tidak valid\n";
        }
    }

    if (command == "getdifficulty")
    {
        const uint64_t target =
            chain.GetLastBlock().header.difficulty;

        const uint64_t initialTarget =
            BerylConsensus::INITIAL_POW_TARGET;

        std::ostringstream out;

        out << "target=" << target << "\n";

        if (target > 0)
        {
            const double difficulty =
                static_cast<double>(initialTarget) /
                static_cast<double>(target);

            out << "difficulty=" << difficulty << "\n";
        }
        else
        {
            out << "difficulty=0\n";
        }

        return out.str();
    }

    // ========================================================
    // GET ADDRESS BALANCE
    //
    // Node tidak menggunakan wallet.
    // RPC hanya membaca UTXO berdasarkan address publik.
    //
    // Format:
    //   getaddressbalance <address>
    // ========================================================

    if (command.rfind("getaddressbalance ", 0) == 0)
    {
        const std::string targetAddress =
            command.substr(
                std::string("getaddressbalance ").size()
            );

        if (targetAddress.empty())
        {
            return
                "ERROR: usage: getaddressbalance <address>\n";
        }

        const uint64_t balance =
            utxoManager.GetBalance(targetAddress);

        std::ostringstream out;

        out << "address="
            << targetAddress
            << "\n";

        out << "balance="
            << FormatBER(balance)
            << "\n";

        return out.str();
    }

    // ========================================================
    // GET ADDRESS UTXOS
    //
    // Wallet standalone menggunakan RPC ini untuk membaca
    // UTXO milik address tanpa memberikan private key ke node.
    //
    // Format:
    //   getaddressutxos <address>
    //
    // Response per baris:
    //   txid=<txid>
    //   index=<index>
    //   amount=<amount>
    //   height=<height>
    //   coinbase=<0|1>
    // ========================================================

    if (command.rfind("getaddressutxos ", 0) == 0)
    {
        const std::string targetAddress =
            command.substr(
                std::string("getaddressutxos ").size()
            );

        if (targetAddress.empty())
        {
            return
                "ERROR: usage: getaddressutxos <address>\n";
        }

        const auto utxos =
            utxoManager.GetUTXOSet().GetByAddress(
                targetAddress
            );

        std::ostringstream out;

        for (const auto& u : utxos)
        {
            out << "txid="
                << u.txid
                << "\n";

            out << "index="
                << u.index
                << "\n";

            out << "amount="
                << FormatBER(u.amount)
                << "\n";

            out << "height="
                << u.height
                << "\n";

            out << "coinbase="
                << (u.coinbase ? 1 : 0)
                << "\n";

            out << "---\n";
        }

        return out.str();
    }



    // ========================================================
    // RESTORE WALLET FROM SEED PHRASE
    //
    // Format:
    //
    // Recovery:
    //   1. Restore index 0 dari seed.
    //   2. Scan seluruh blockchain.
    //   3. Derive address index secara deterministik.
    //   4. Pulihkan address yang pernah digunakan.
    // ========================================================







    // ========================================================
    // GET TRANSACTION
    // Format:
    //   gettransaction <txid>
    //
    // Mencari transaksi di seluruh blockchain.
    // Fee dihitung berdasarkan input historis.
    // ========================================================
    if (command.rfind("gettransaction ", 0) == 0)
    {
        std::istringstream iss(command);

        std::string method;
        std::string txid;
        std::string extra;

        iss >> method >> txid;

        if (txid.empty())
            return "ERROR: usage: gettransaction <txid>\n";

        if (iss >> extra)
            return "ERROR: terlalu banyak argumen\n";

        BerylTransaction tx;
        int blockHeight = -1;

        // ----------------------------------------------------
        // Cari di blockchain confirmed.
        // ----------------------------------------------------
        if (FindTransactionInChain(
                chain,
                txid,
                tx,
                blockHeight))
        {
            const int currentHeight = chain.GetHeight();

            int confirmations = 0;

            if (blockHeight > 0 &&
                currentHeight >= blockHeight)
            {
                confirmations =
                    currentHeight - blockHeight + 1;
            }

            const uint64_t fee =
                CalculateHistoricalTransactionFee(
                    chain,
                    tx
                );

            std::ostringstream out;

            out << "TXID=" << tx.txid << "\n";
            out << "STATUS=confirmed\n";
            out << "BLOCK_HEIGHT=" << blockHeight << "\n";
            out << "CONFIRMATIONS=" << confirmations << "\n";

            // ------------------------------------------------
            // FROM
            // ------------------------------------------------
            if (!tx.vin.empty())
            {
                TxOutput previousOutput;

                if (FindHistoricalOutput(
                        chain,
                        tx.vin[0].previousTx,
                        tx.vin[0].outputIndex,
                        previousOutput))
                {
                    out << "FROM="
                        << previousOutput.address
                        << "\n";
                }
            }

            // ------------------------------------------------
            // OUTPUTS
            // ------------------------------------------------
            for (size_t i = 0; i < tx.vout.size(); ++i)
            {
                out << "OUTPUT_" << i
                    << "_ADDRESS="
                    << tx.vout[i].address
                    << "\n";

                out << "OUTPUT_" << i
                    << "_AMOUNT="
                    << FormatBER(tx.vout[i].amount)
                    << "\n";
            }

            if (!tx.vout.empty())
            {
                out << "TO="
                    << tx.vout[0].address
                    << "\n";

                out << "AMOUNT="
                    << FormatBER(tx.vout[0].amount)
                    << "\n";
            }

            out << "FEE="
                << FormatBER(fee)
                << "\n";

            out << "INPUT_COUNT="
                << tx.vin.size()
                << "\n";

            out << "OUTPUT_COUNT="
                << tx.vout.size()
                << "\n";

            return out.str();
        }

        // ----------------------------------------------------
        // Belum confirmed / tidak ditemukan di blockchain.
        // Cek mempool.
        // ----------------------------------------------------
        for (const auto& memTx : mempool.GetTransactions())
        {
            if (memTx.txid == txid)
            {
                std::ostringstream out;

                out << "TXID=" << memTx.txid << "\n";
                out << "STATUS=unconfirmed\n";
                out << "BLOCK_HEIGHT=0\n";
                out << "CONFIRMATIONS=0\n";

                if (!memTx.vin.empty())
                {
                    TxOutput previousOutput;

                    if (FindHistoricalOutput(
                            chain,
                            memTx.vin[0].previousTx,
                            memTx.vin[0].outputIndex,
                            previousOutput))
                    {
                        out << "FROM="
                            << previousOutput.address
                            << "\n";
                    }
                }

                for (size_t i = 0;
                     i < memTx.vout.size();
                     ++i)
                {
                    out << "OUTPUT_" << i
                        << "_ADDRESS="
                        << memTx.vout[i].address
                        << "\n";

                    out << "OUTPUT_" << i
                        << "_AMOUNT="
                        << FormatBER(memTx.vout[i].amount)
                        << "\n";
                }

                if (!memTx.vout.empty())
                {
                    out << "TO="
                        << memTx.vout[0].address
                        << "\n";

                    out << "AMOUNT="
                        << FormatBER(memTx.vout[0].amount)
                        << "\n";
                }

                const uint64_t fee =
                    CalculateHistoricalTransactionFee(
                        chain,
                        memTx
                    );

                out << "FEE="
                    << FormatBER(fee)
                    << "\n";

                out << "INPUT_COUNT="
                    << memTx.vin.size()
                    << "\n";

                out << "OUTPUT_COUNT="
                    << memTx.vout.size()
                    << "\n";

                return out.str();
            }
        }

        return "ERROR: transaction not found\n";
    }





    // ============================================================
    // SEND TO ADDRESS
    // Format:
    //
    // Amount menggunakan 8 decimal places:
    //   1       = 1 BER
    //   1.5     = 1.50000000 BER
    // ============================================================



    // ============================================================
    // SUBMIT TRANSACTION
    //
    // Wallet membuat dan menandatangani transaksi secara lokal.
    // Node hanya menerima transaksi yang sudah ditandatangani.
    //
    // Private key TIDAK pernah dikirim ke node.
    //
    // Format:
    //   submittransaction <txhex>
    // ============================================================

    if (command.rfind("submittransaction ", 0) == 0)
    {
        const std::string txHex =
            command.substr(
                std::string("submittransaction ").size()
            );

        if (txHex.empty())
            return "ERROR: usage: submittransaction <txhex>\n";

        std::string serialized;

        if (!HexToBytes(
                txHex,
                serialized))
        {
            return "ERROR: invalid transaction hex\n";
        }

        BerylTransaction tx;

        if (!DeserializeBerylTransaction(
                serialized,
                tx))
        {
            return "ERROR: transaction deserialization failed\n";
        }

        if (tx.vin.empty())
            return "ERROR: coinbase transaction cannot enter mempool\n";

        if (tx.txid.empty())
            return "ERROR: transaction TXID kosong\n";

        // Pastikan TXID sesuai isi transaksi.
        if (CalculateTxID(tx) != tx.txid)
        {
            return "ERROR: transaction TXID mismatch\n";
        }

        // Falcon signature wajib valid.
        if (!VerifyTransactionSignature(tx))
        {
            return "ERROR: invalid Falcon transaction signature\n";
        }

        const int currentHeight =
            chain.GetHeight();

        // Validasi consensus transaksi.
        if (!ValidateTransaction(
                utxoManager.GetUTXOSet(),
                tx,
                currentHeight))
        {
            return "ERROR: transaction validation failed\n";
        }

        // Masukkan ke mempool.
        if (!mempool.AddTransaction(
                utxoManager.GetUTXOSet(),
                tx,
                currentHeight))
        {
            return "ERROR: transaction rejected by mempool\n";
        }

        // Relay ke peer.
        BroadcastTransaction(tx);

        std::ostringstream out;

        out << "ACCEPTED\n";
        out << "TXID="
            << tx.txid
            << "\n";
        out << "STATUS=unconfirmed\n";

        return out.str();
    }


    // ============================================================
    // SUBMIT BLOCK
    // ============================================================
    // Miner eksternal mengirim block lengkap dalam hexadecimal.
    //
    // Node bertanggung jawab untuk:
    //   1. decode hexadecimal
    //   2. deserialize block
    //   3. validasi consensus
    //   4. memasukkan block ke chain
    //   5. menyimpan blockchain
    //   6. relay block ke peer
    //
    // Miner TIDAK pernah memanggil chain.AddBlock() langsung.
    // ============================================================

    if (command.rfind("submitblock ", 0) == 0)
    {
        const std::string blockHex =
            command.substr(
                std::string("submitblock ").size()
            );

        if (blockHex.empty())
            return "ERROR: usage: submitblock <blockhex>\n";

        std::string serialized;

        if (!HexToBytes(
                blockHex,
                serialized))
        {
            return "ERROR: invalid block hex\n";
        }

        BerylBlock block;

        if (!DeserializeBerylBlock(
                serialized,
                block))
        {
            return "ERROR: block deserialization failed\n";
        }

        // --------------------------------------------------------
        // Validasi consensus.
        // --------------------------------------------------------

        if (!ValidateBlock(
                block,
                chain,
                utxoManager))
        {
            return "REJECTED: block validation failed\n";
        }

        // --------------------------------------------------------
        // Masukkan block ke chain.
        // AddBlock() kembali melakukan validasi internal.
        // --------------------------------------------------------

        if (!chain.AddBlock(block))
        {
            return "REJECTED: block rejected by chain\n";
        }

        // --------------------------------------------------------
        // Simpan blockchain.
        // --------------------------------------------------------

        // blockchainFile berasal dari datadir node.

        if (!SaveBlockchain(
                chain,
                blockchainFile))
        {
            std::cerr
                << "WARNING: submitted block accepted "
                << "but blockchain save failed\n";
        }

        // --------------------------------------------------------
        // Block resmi diterima.
        // --------------------------------------------------------

        BroadcastBlock(block);

        std::ostringstream out;

        out << "ACCEPTED\n";
        out << "HEIGHT="
            << block.header.height
            << "\n";
        out << "HASH="
            << block.hash
            << "\n";
        out << "NONCE="
            << block.header.nonce
            << "\n";

        return out.str();
    }





    if (command == "getmininginfo")
    {
        std::ostringstream out;

        const int height = chain.GetHeight();

        const uint64_t reward =
            BerylConsensus::GetBlockSubsidy(height + 1);

        out << "blocks="
            << height
            << "\n";

        out << "pow=yespower\n";

        out << "next_reward="
            << FormatBER(reward)
            << "\n";

        out << "peers="
            << GetActivePeerCount()
            << "\n";

        out << "node_mining=false\n";

        return out.str();
    }

    return "ERROR: unknown command\n";
}

static void RPCServer(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool,
    const std::string& blockchainFile,
    int rpcPort
)
{
    int serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0)
    {
        std::cerr
            << "RPC ERROR: socket failed\n";

        return;
    }

    int opt = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    sockaddr_in server{};

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server.sin_port = htons(rpcPort);

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&server),
            sizeof(server)
        ) < 0)
    {
        std::cerr
            << "RPC ERROR: bind failed on port "
            << rpcPort
            << "\n";

        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 16) < 0)
    {
        std::cerr
            << "RPC ERROR: listen failed\n";

        close(serverSocket);
        return;
    }

    std::cout
        << "RPC SERVER LISTENING ON 127.0.0.1:"
        << rpcPort
        << "\n";

    while (true)
    {
        sockaddr_in client{};
        socklen_t clientLen = sizeof(client);

        int clientSocket = accept(
            serverSocket,
            reinterpret_cast<sockaddr*>(&client),
            &clientLen
        );

        if (clientSocket < 0)
            continue;

        std::string command;

        char buffer[4096];

        // TCP adalah stream. Baca sampai client selesai
        // mengirim seluruh command.
        while (true)
        {
            const ssize_t received =
                recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0
                );

            if (received <= 0)
                break;

            command.append(
                buffer,
                static_cast<size_t>(received)
            );
        }

        while (
            !command.empty() &&
            (
                command.back() == '\n' ||
                command.back() == '\r'
            )
        )
        {
            command.pop_back();
        }

        if (!command.empty())
        {
            std::string response =
                ProcessRPC(
                    command,
                    chain,
                    utxoManager,
                    mempool,
                    blockchainFile
                );

            // Kirim seluruh response.
            size_t sent = 0;

            while (sent < response.size())
            {
                const ssize_t n =
                    send(
                        clientSocket,
                        response.data() + sent,
                        response.size() - sent,
                        0
                    );

                if (n <= 0)
                    break;

                sent += static_cast<size_t>(n);
            }
        }

        close(clientSocket);
    }
}

void StartRPC(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool,
    const std::string& blockchainFile
)
{
    const int rpcPort = GetRPCPort();

    std::thread(
        RPCServer,
        std::ref(chain),
        std::ref(utxoManager),
        std::ref(mempool),
        std::cref(blockchainFile),
        rpcPort
    ).detach();

    // Informasi RPC milik NODE.
    // Tidak ada wallet address, balance, seed,
    // atau private key di sini.

    std::ofstream file("../data/rpc.info");

    if (!file)
        return;

    file << "chain=main\n";
    file << "node=fullnode\n";
    file << "pow=yespower\n";
    file << "wallet=false\n";
    file << "miner=false\n";
    file << "rpc=127.0.0.1:" << rpcPort << "\n";

    file.close();
}
