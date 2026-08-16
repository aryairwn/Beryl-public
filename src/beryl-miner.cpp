#include "block.h"
#include "blockhash.h"
#include "yespower_wrapper.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <csignal>

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

static constexpr const char* RPC_HOST = "127.0.0.1";
static constexpr int RPC_PORT = 18444;

static constexpr const char* MINER_STATUS_FILE = "miner.status";

static void WriteMinerStatus(unsigned int workers)
{
    std::ofstream status(
        MINER_STATUS_FILE,
        std::ios::trunc
    );

    if (!status)
        return;

    status
        << "running=1\n"
        << "workers="
        << workers
        << "\n";
}

static void ClearMinerStatus()
{
    std::remove(MINER_STATUS_FILE);
}

static void MinerSignalHandler(int)
{
    ClearMinerStatus();
    std::_Exit(0);
}


static bool CheckMiningTarget(
    const std::string& hash,
    uint64_t target
)
{
    if (target == 0 || hash.size() < 16)
        return false;

    uint64_t value = 0;

    for (int i = 0; i < 16; ++i)
    {
        const char c = hash[i];

        uint64_t nibble;

        if (c >= '0' && c <= '9')
            nibble = c - '0';
        else if (c >= 'a' && c <= 'f')
            nibble = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            nibble = c - 'A' + 10;
        else
            return false;

        value = (value << 4) | nibble;
    }

    return value <= target;
}

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
        out.push_back(hex[c & 15]);
    }

    return out;
}

static int ConnectRPC()
{
    int sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sock < 0)
        return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RPC_PORT);

    if (inet_pton(
            AF_INET,
            RPC_HOST,
            &addr.sin_addr) != 1)
    {
        close(sock);
        return -1;
    }

    if (connect(
            sock,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) < 0)
    {
        close(sock);
        return -1;
    }

    return sock;
}

static bool CallRPC(
    const std::string& command,
    std::string& response
)
{
    const int sock = ConnectRPC();

    if (sock < 0)
        return false;

    // --------------------------------------------------------
    // Kirim SELURUH command.
    //
    // send() tidak menjamin seluruh buffer terkirim sekaligus.
    // Sangat penting untuk submitblock karena block HEX
    // bisa berukuran besar.
    // --------------------------------------------------------

    size_t sent = 0;

    while (sent < command.size())
    {
        const ssize_t n =
            send(
                sock,
                command.data() + sent,
                command.size() - sent,
                0
            );

        if (n <= 0)
        {
            close(sock);
            return false;
        }

        sent += static_cast<size_t>(n);
    }

    // Beri tanda bahwa seluruh command sudah dikirim.
    shutdown(sock, SHUT_WR);

    // --------------------------------------------------------
    // Terima seluruh response RPC.
    // --------------------------------------------------------

    char buffer[8192];

    response.clear();

    while (true)
    {
        const ssize_t n =
            recv(
                sock,
                buffer,
                sizeof(buffer),
                0
            );

        if (n <= 0)
            break;

        response.append(
            buffer,
            static_cast<size_t>(n)
        );

        if (static_cast<size_t>(n) < sizeof(buffer))
            break;
    }

    close(sock);

    return !response.empty();
}

static std::string GetValue(
    const std::string& response,
    const std::string& key
)
{
    const std::string prefix =
        key + "=";

    const size_t pos =
        response.find(prefix);

    if (pos == std::string::npos)
        return "";

    const size_t start =
        pos + prefix.size();

    const size_t end =
        response.find(
            '\n',
            start);

    if (end == std::string::npos)
        return response.substr(start);

    return response.substr(
        start,
        end - start);
}

static bool GetTemplate(
    const std::string& address,
    BerylBlock& block
)
{
    std::string response;

    const std::string command =
        "getblocktemplate " + address;

    if (!CallRPC(
            command,
            response))
    {
        std::cerr
            << "ERROR: CallRPC FAILED\n";
        return false;
    }

    if (response.empty())
    {
        std::cerr
            << "ERROR: RPC RESPONSE EMPTY\n";
        return false;
    }

    if (response.rfind("ERROR:", 0) == 0)
    {
        std::cerr
            << "ERROR: RPC RETURNED ERROR\n"
            << response;
        return false;
    }

    const std::string hex =
        GetValue(
            response,
            "blockhex");

    if (hex.empty())
    {
        std::cerr
            << "ERROR: BLOCKHEX EMPTY\n"
            << "----- RPC RESPONSE -----\n"
            << response
            << "----- END RESPONSE -----\n";
        return false;
    }

    if (hex.size() % 2 != 0)
    {
        std::cerr
            << "ERROR: BLOCKHEX ODD LENGTH\n";
        return false;
    }

    std::string serialized;
    serialized.reserve(hex.size() / 2);

    for (size_t i = 0;
         i < hex.size();
         i += 2)
    {
        auto hexValue =
            [](char c) -> int
        {
            if (c >= '0' && c <= '9')
                return c - '0';

            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;

            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;

            return -1;
        };

        const int hi = hexValue(hex[i]);
        const int lo = hexValue(hex[i + 1]);

        if (hi < 0 || lo < 0)
        {
            std::cerr
                << "ERROR: INVALID HEX AT "
                << i
                << "\n";
            return false;
        }

        serialized.push_back(
            static_cast<char>(
                (hi << 4) | lo
            )
        );
    }

    if (!DeserializeBerylBlock(
            serialized,
            block))
    {
        std::cerr
            << "ERROR: DeserializeBerylBlock FAILED\n"
            << "HEX_SIZE="
            << hex.size()
            << "\n"
            << "BIN_SIZE="
            << serialized.size()
            << "\n";

        return false;
    }

    return true;
}

static bool SubmitBlock(
    const BerylBlock& block
)
{
    const std::string serialized =
        SerializeBerylBlock(block);

    const std::string hex =
        BytesToHex(serialized);

    std::string response;

    if (!CallRPC(
            "submitblock " + hex,
            response))
    {
        return false;
    }

    std::cout
        << response;

    return response.find(
        "ACCEPTED") !=
        std::string::npos;
}

static bool MineTemplate(
    BerylBlock& block,
    unsigned int workers
)
{
    if (workers == 0)
        workers = 1;

    std::atomic<bool> found{false};

    std::atomic<uint32_t> foundNonce{0};

    std::string foundHash;

    std::mutex mutex;

    std::vector<std::thread> threads;

    threads.reserve(workers);

    for (unsigned int worker = 0;
         worker < workers;
         ++worker)
    {
        threads.emplace_back(
            [&block,
             worker,
             workers,
             &found,
             &foundNonce,
             &foundHash,
             &mutex]()
            {
                for (
                    uint64_t nonce = worker;
                    nonce <= UINT32_MAX;
                    nonce += workers)
                {
                    if (found.load(
                            std::memory_order_acquire))
                    {
                        break;
                    }

                    BerylHeader header =
                        block.header;

                    header.nonce =
                        static_cast<uint32_t>(
                            nonce);

                    const std::string hash =
                        GetBlockHash(header);

                    if (!CheckMiningTarget(
                            hash,
                            header.difficulty))
                    {
                        continue;
                    }

                    bool expected = false;

                    if (found.compare_exchange_strong(
                            expected,
                            true,
                            std::memory_order_acq_rel))
                    {
                        std::lock_guard<std::mutex>
                            lock(mutex);

                        foundNonce.store(
                            header.nonce,
                            std::memory_order_release);

                        foundHash = hash;
                    }

                    break;
                }
            });
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    if (!found.load(
            std::memory_order_acquire))
    {
        return false;
    }

    block.header.nonce =
        foundNonce.load(
            std::memory_order_acquire);

    {
        std::lock_guard<std::mutex>
            lock(mutex);

        block.hash = foundHash;
    }

    return true;
}

int main(
    int argc,
    char* argv[]
)
{
    unsigned int workers = 1;

    if (argc >= 2)
    {
        try
        {
            const unsigned long value =
                std::stoul(argv[1]);

            if (value == 0 ||
                value > 64)
            {
                std::cerr
                    << "usage: ./beryl-miner <threads>\n";

                return 1;
            }

            workers =
                static_cast<unsigned int>(
                    value);
        }
        catch (...)
        {
            std::cerr
                << "usage: ./beryl-miner <threads>\n";

            return 1;
        }
    }

    std::cout
        << "BERYL MINER\n"
        << "workers="
        << workers
        << "\n";

    // Status miner eksternal untuk getmininginfo.
    WriteMinerStatus(workers);

    std::signal(SIGINT, MinerSignalHandler);
    std::signal(SIGTERM, MinerSignalHandler);

    // ========================================================
    // MINING REWARD ADDRESS
    // ========================================================
    // Miner TIDAK memiliki wallet, private key, atau seed.
    //
    // Pemakaian:
    //
    // Pertama kali:
    //   ./beryl-miner 4 <beryl-address>
    //
    // Berikutnya:
    //   ./beryl-miner 4
    //
    // Address publik disimpan di miner.conf.
    // ========================================================

    const std::string minerConfigFile =
        "miner.conf";

    std::string address;

    if (argc >= 3)
    {
        address = argv[2];

        if (!IsValidBerylAddress(address))
        {
            std::cerr
                << "ERROR: invalid Beryl mining address\n";

            return 1;
        }

        std::ofstream config(
            minerConfigFile,
            std::ios::trunc
        );

        if (!config)
        {
            std::cerr
                << "ERROR: gagal menyimpan miner.conf\n";

            return 1;
        }

        config
            << "address="
            << address
            << "\n";

        config.close();

        std::cout
            << "MINER ADDRESS SAVED\n";
    }
    else
    {
        std::ifstream config(
            minerConfigFile
        );

        std::string line;

        while (std::getline(config, line))
        {
            const std::string prefix =
                "address=";

            if (line.rfind(prefix, 0) == 0)
            {
                address =
                    line.substr(prefix.size());

                break;
            }
        }

        config.close();

        if (!IsValidBerylAddress(address))
        {
            std::cerr
                << "ERROR: miner.conf contains invalid Beryl address\n";

            return 1;
        }

        std::cout
            << "MINER ADDRESS LOADED\n";
    }

    std::cout
        << "MINER ADDRESS: "
        << address
        << "\n";

    while (true)
    {
        BerylBlock block;

        if (!GetTemplate(
                address,
                block))
        {
            std::cerr
                << "ERROR: gagal mengambil block template\n";

            std::this_thread::sleep_for(
                std::chrono::seconds(1));

            continue;
        }

        std::cout
            << "MINING HEIGHT="
            << block.header.height
            << " DIFFICULTY="
            << block.header.difficulty
            << "\n";

        if (!MineTemplate(
                block,
                workers))
        {
            std::cerr
                << "MINING FAILED\n";

            continue;
        }

        std::cout
            << "BLOCK FOUND\n"
            << "HEIGHT="
            << block.header.height
            << "\n"
            << "NONCE="
            << block.header.nonce
            << "\n"
            << "HASH="
            << block.hash
            << "\n";

        if (!SubmitBlock(block))
        {
            std::cout
                << "BLOCK REJECTED/STALE\n";
        }
    }

    return 0;
}
