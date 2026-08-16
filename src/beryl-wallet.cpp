#include "wallet.h"
#include "transaction.h"
#include "utxo.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

static constexpr const char* RPC_HOST = "127.0.0.1";
static constexpr int RPC_PORT = 18444;

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

static bool CallRPC(
    const std::string& command,
    std::string& response
)
{
    const int sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sock < 0)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RPC_PORT);

    if (inet_pton(
            AF_INET,
            RPC_HOST,
            &addr.sin_addr) != 1)
    {
        close(sock);
        return false;
    }

    if (connect(
            sock,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) < 0)
    {
        close(sock);
        return false;
    }

    // --------------------------------------------------------
    // Kirim seluruh RPC command.
    // TCP adalah stream, send() tidak menjamin seluruh
    // buffer terkirim dalam satu kali pemanggilan.
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

    // RPC server membaca command sampai EOF.
    // Socket tetap terbuka untuk menerima response.
    shutdown(sock, SHUT_WR);

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

        if (static_cast<size_t>(n) <
            sizeof(buffer))
        {
            break;
        }
    }

    close(sock);

    return !response.empty();
}

static bool GetNodeBlockCount(
    std::string& result
)
{
    std::string response;

    if (!CallRPC(
            "getblockcount",
            response))
    {
        return false;
    }

    result = response;
    return true;
}

static const std::string WALLET_ROOT = "../wallet";
static const std::string ACTIVE_FILE = "../wallet/active-wallet";

static bool EnsureWalletRoot()
{
    return system(
        "mkdir -p ../wallet"
    ) == 0;
}

static std::string GetActiveWalletName()
{
    std::ifstream file(ACTIVE_FILE);

    std::string name;

    if (file && std::getline(file, name))
        return name;

    return "";
}

static bool SetActiveWallet(
    const std::string& name
)
{
    std::ofstream file(ACTIVE_FILE);

    if (!file)
        return false;

    file << name << "\n";

    return true;
}

static std::string WalletPath(
    const std::string& name
)
{
    return WALLET_ROOT + "/" + name + "/wallet.dat";
}

static bool WalletExists(
    const std::string& name
)
{
    std::ifstream file(
        WalletPath(name)
    );

    return file.good();
}

static int WalletNumber(
    const std::string& name
)
{
    if (name.rfind("wallet-", 0) != 0)
        return -1;

    try
    {
        return std::stoi(
            name.substr(7)
        );
    }
    catch (...)
    {
        return -1;
    }
}

static std::string FindNextWalletName()
{
    int number = 1;

    while (
        WalletExists(
            "wallet-" +
            std::to_string(number)))
    {
        ++number;
    }

    return
        "wallet-" +
        std::to_string(number);
}

static bool CreateDirectory(
    const std::string& path
)
{
    std::string command =
        "mkdir -p \"" +
        path +
        "\"";

    return system(
        command.c_str()
    ) == 0;
}

static bool LoadActiveWallet(
    BerylWallet& wallet,
    std::string& walletName,
    std::string& walletFile
)
{
    walletName =
        GetActiveWalletName();

    if (walletName.empty())
    {
        std::cerr
            << "ERROR: no active wallet\n"
            << "Use: ./beryl-wallet use wallet-1\n";

        return false;
    }

    walletFile =
        WalletPath(walletName);

    if (!wallet.Load(walletFile))
    {
        std::cerr
            << "ERROR: wallet invalid: "
            << walletFile
            << "\n";

        return false;
    }

    return true;
}

static void PrintUsage()
{
    std::cout
        << "BERYL WALLET\n"
        << "\n"
        << "Wallet management:\n"
        << "  ./beryl-wallet create\n"
        << "  ./beryl-wallet recover \"<24-word-seed>\"\n"
        << "  ./beryl-wallet list\n"
        << "  ./beryl-wallet use <wallet-name>\n"
        << "\n"
        << "Active wallet:\n"
        << "  ./beryl-wallet address\n"
        << "  ./beryl-wallet newaddress\n"
        << "  ./beryl-wallet addresses\n"
        << "  ./beryl-wallet balance\n"
        << "  ./beryl-wallet send <address> <amount>\n"
        << "  ./beryl-wallet seed\n"
        << "  ./beryl-wallet info\n"
        << "  ./beryl-wallet getblockcount\n";
}

int main(
    int argc,
    char* argv[]
)
{
    if (!EnsureWalletRoot())
    {
        std::cerr
            << "ERROR: failed to create wallet directory\n";

        return 1;
    }

    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    const std::string command =
        argv[1];

    // ========================================================
    // CREATE NEW WALLET
    // ========================================================

    if (command == "create")
    {
        const std::string walletName =
            FindNextWalletName();

        const std::string directory =
            WALLET_ROOT + "/" + walletName;

        const std::string walletFile =
            WalletPath(walletName);

        if (!CreateDirectory(directory))
        {
            std::cerr
                << "ERROR: failed to create wallet directory\n";

            return 1;
        }

        BerylWallet wallet;

        if (!wallet.Generate())
        {
            std::cerr
                << "ERROR: failed to generate wallet\n";

            return 1;
        }

        if (!wallet.Save(walletFile))
        {
            std::cerr
                << "ERROR: failed to save wallet\n";

            return 1;
        }

        if (!SetActiveWallet(walletName))
        {
            std::cerr
                << "ERROR: failed to set active wallet\n";

            return 1;
        }

        std::cout
            << "BERYL WALLET CREATED\n"
            << "WALLET: "
            << walletName
            << "\n"
            << "WALLET FILE: "
            << walletFile
            << "\n"
            << "ADDRESS: "
            << wallet.GetAddress()
            << "\n"
            << "SEED PHRASE: "
            << wallet.GetSeedPhrase()
            << "\n"
            << "STATUS: ACTIVE\n";

        return 0;
    }

    // ========================================================
    // RECOVER WALLET FROM BERYL SEED V1
    // ========================================================

    if (command == "recover")
    {
        // ========================================================
        // RECOVER WALLET FROM BERYL SEED V1
        //
        // Recovery WAJIB menerima tepat 24 kata sebagai
        // argument terpisah.
        //
        // Contoh:
        // ./beryl-wallet recover word1 word2 ... word24
        //
        // Seed tidak boleh diberikan sebagai satu string
        // dan tidak ada automatic recovery dari command
        // ./beryl-wallet seed.
        // ========================================================

        if (argc != 26)
        {
            std::cerr
                << "ERROR: usage: ./beryl-wallet recover "
                << "<word1> <word2> ... <word24>\n";
            return 1;
        }

        std::vector<std::string> words;

        for (int i = 2; i < 26; ++i)
        {
            words.push_back(argv[i]);
        }


        if (!BerylSeed::ValidateMnemonic(words))
        {
            std::cerr
                << "ERROR: seed phrase checksum tidak valid\n";

            return 1;
        }

        // Cari nama wallet baru.
        const std::string walletName =
            FindNextWalletName();

        const std::string directory =
            WALLET_ROOT + "/" + walletName;

        const std::string walletFile =
            WalletPath(walletName);

        if (!CreateDirectory(directory))
        {
            std::cerr
                << "ERROR: failed to create wallet directory\n";

            return 1;
        }

        BerylWallet wallet;

        if (!wallet.RestoreFromMnemonic(words))
        {
            std::cerr
                << "ERROR: gagal melakukan recovery wallet\n";

            return 1;
        }

        // Simpan wallet hasil recovery.
        if (!wallet.Save(walletFile))
        {
            std::cerr
                << "ERROR: failed to save recovered wallet\n";

            return 1;
        }

        // Jadikan wallet hasil recovery sebagai active wallet.
        if (!SetActiveWallet(walletName))
        {
            std::cerr
                << "ERROR: failed to set active wallet\n";

            return 1;
        }

        std::cout
            << "BERYL WALLET RECOVERED\n"
            << "WALLET: "
            << walletName
            << "\n"
            << "WALLET FILE: "
            << walletFile
            << "\n"
            << "ADDRESS: "
            << wallet.GetAddress()
            << "\n"
            << "STATUS: ACTIVE\n";

        return 0;
    }

    // ========================================================
    // LIST WALLETS
    // ========================================================

    if (command == "list")
    {
        const std::string active =
            GetActiveWalletName();

        std::cout
            << "BERYL WALLETS\n";

        bool found = false;

        for (int i = 1; i <= 10000; ++i)
        {
            const std::string name =
                "wallet-" +
                std::to_string(i);

            if (!WalletExists(name))
                continue;

            found = true;

            std::cout
                << (name == active ? "* " : "  ")
                << name
                << "\n";
        }

        if (!found)
        {
            std::cout
                << "No wallets found.\n";
        }

        return 0;
    }

    // ========================================================
    // USE WALLET
    // ========================================================

    if (command == "use")
    {
        if (argc < 3)
        {
            std::cerr
                << "ERROR: usage: ./beryl-wallet use <wallet-name>\n";

            return 1;
        }

        const std::string walletName =
            argv[2];

        if (!WalletExists(walletName))
        {
            std::cerr
                << "ERROR: wallet not found: "
                << walletName
                << "\n";

            return 1;
        }

        if (!SetActiveWallet(walletName))
        {
            std::cerr
                << "ERROR: failed to select wallet\n";

            return 1;
        }

        std::cout
            << "ACTIVE WALLET: "
            << walletName
            << "\n";

        return 0;
    }

    // ========================================================
    // LOAD ACTIVE WALLET
    // ========================================================

    BerylWallet wallet;

    std::string walletName;
    std::string walletFile;

    if (!LoadActiveWallet(
            wallet,
            walletName,
            walletFile))
    {
        return 1;
    }

    // ========================================================
    // ADDRESS
    // ========================================================

    if (command == "address")
    {
        std::cout
            << wallet.GetAddress()
            << "\n";

        return 0;
    }

    // ========================================================
    // NEW ADDRESS
    // ========================================================

    if (command == "newaddress")
    {
        std::string newAddress;

        if (!wallet.GenerateNewAddress(
                newAddress))
        {
            std::cerr
                << "ERROR: failed to generate new address\n";

            return 1;
        }

        if (!wallet.Save(walletFile))
        {
            std::cerr
                << "ERROR: failed to save wallet\n";

            return 1;
        }

        std::cout
            << "NEW ADDRESS: "
            << newAddress
            << "\n";

        return 0;
    }

    // ========================================================
    // ALL ADDRESSES
    // ========================================================

    if (command == "addresses")
    {
        const auto addresses =
            wallet.GetAddresses();

        std::cout
            << "WALLET: "
            << walletName
            << "\n"
            << "ADDRESSES: "
            << addresses.size()
            << "\n";

        for (size_t i = 0;
             i < addresses.size();
             ++i)
        {
            std::cout
                << i
                << ": "
                << addresses[i]
                << "\n";
        }

        return 0;
    }
    // ========================================================
    // SEND TO ADDRESS
    //
    // Wallet:
    //   1. mengambil UTXO dari node
    //   2. membuat transaksi
    //   3. signing dengan Falcon secara lokal
    //   4. serialize transaksi
    //   5. submit transaction ke node
    //
    // Private key TIDAK pernah dikirim ke node.
    // ========================================================

    if (command == "send")
    {
        if (argc < 4)
        {
            std::cerr
                << "ERROR: usage: ./beryl-wallet send <address> <amount>\n";

            return 1;
        }

        if (argc > 4)
        {
            std::cerr
                << "ERROR: terlalu banyak argumen\n";

            return 1;
        }

        const std::string to = argv[2];
        const std::string amountText = argv[3];

        if (to.rfind("ber", 0) != 0 ||
            to.size() <= 3)
        {
            std::cerr
                << "ERROR: invalid Beryl address\n";

            return 1;
        }

        // ----------------------------------------------------
        // Parse amount.
        // 1 BER = 100000000 unit.
        // ----------------------------------------------------

        uint64_t amount = 0;

        try
        {
            const size_t dot = amountText.find('.');

            uint64_t whole = 0;
            uint64_t fraction = 0;

            if (dot == std::string::npos)
            {
                whole = std::stoull(amountText);
            }
            else
            {
                whole = std::stoull(
                    amountText.substr(0, dot)
                );

                std::string frac =
                    amountText.substr(dot + 1);

                if (frac.size() > 8)
                {
                    throw std::runtime_error(
                        "terlalu banyak desimal"
                    );
                }

                while (frac.size() < 8)
                    frac.push_back('0');

                fraction = std::stoull(frac);
            }

            if (whole >
                (UINT64_MAX - fraction) / 100000000ULL)
            {
                throw std::runtime_error(
                    "amount overflow"
                );
            }

            amount =
                whole * 100000000ULL +
                fraction;

            if (amount == 0)
            {
                throw std::runtime_error(
                    "amount harus lebih besar dari 0"
                );
            }
        }
        catch (...)
        {
            std::cerr
                << "ERROR: amount tidak valid\n";

            return 1;
        }

        // Buat transaksi.
        //
        // Coba setiap address wallet sampai ditemukan
        // address yang mempunyai UTXO cukup.
        // ----------------------------------------------------

        
        // ----------------------------------------------------
        // Ambil seluruh UTXO wallet dari node.
        // ----------------------------------------------------
        UTXOSet walletUTXOs;

        const auto addresses =
            wallet.GetAddresses();

        for (const auto& address :
             addresses)
        {
            std::string response;

            if (!CallRPC(
                    "getaddressutxos " + address,
                    response))
            {
                std::cerr
                    << "ERROR: gagal mengambil UTXO\n";

                return 1;
            }

            if (response.rfind(
                    "ERROR:",
                    0) == 0)
            {
                std::cerr
                    << response;

                return 1;
            }

            std::istringstream iss(response);
            std::string line;

            UTXO u;
            u.address = address;

            bool haveTxid = false;
            bool haveIndex = false;
            bool haveAmount = false;
            bool haveHeight = false;
            bool haveCoinbase = false;

            auto addCurrent = [&]()
            {
                if (haveTxid &&
                    haveIndex &&
                    haveAmount &&
                    haveHeight &&
                    haveCoinbase)
                {
                    u.address = address;
                    walletUTXOs.Add(u);
                }

                u = UTXO();
                u.address = address;

                haveTxid = false;
                haveIndex = false;
                haveAmount = false;
                haveHeight = false;
                haveCoinbase = false;
            };

            while (std::getline(iss, line))
            {
                if (line == "---")
                {
                    addCurrent();
                    continue;
                }

                const size_t eq =
                    line.find('=');

                if (eq == std::string::npos)
                    continue;

                const std::string keyName =
                    line.substr(0, eq);

                const std::string value =
                    line.substr(eq + 1);

                try
                {
                    if (keyName == "txid")
                    {
                        u.txid = value;
                        haveTxid = true;
                    }
                    else if (keyName == "index")
                    {
                        u.index =
                            static_cast<uint32_t>(
                                std::stoul(value)
                            );
                        haveIndex = true;
                    }
                    else if (keyName == "amount")
                    {
                        u.amount =
                            std::stoull(value);
                        haveAmount = true;
                    }
                    else if (keyName == "height")
                    {
                        u.height =
                            std::stoi(value);
                        haveHeight = true;
                    }
                    else if (keyName == "coinbase")
                    {
                        u.coinbase =
                            (value == "1");
                        haveCoinbase = true;
                    }
                }
                catch (...)
                {
                    std::cerr
                        << "ERROR: data UTXO tidak valid\n";

                    return 1;
                }
            }

            addCurrent();
        }

        // ----------------------------------------------------
        // Tinggi blockchain saat transaksi dibuat.
        // ----------------------------------------------------
        int currentHeight = -1;

        {
            std::string heightResponse;

            if (!CallRPC(
                    "getblockcount",
                    heightResponse))
            {
                std::cerr
                    << "ERROR: gagal mengambil block height\n";

                return 1;
            }

            try
            {
                const size_t eq =
                    heightResponse.find('=');

                if (eq == std::string::npos)
                {
                    throw std::runtime_error(
                        "format block height tidak valid"
                    );
                }

                currentHeight =
                    std::stoi(
                        heightResponse.substr(eq + 1)
                    );
            }
            catch (...)
            {
                std::cerr
                    << "ERROR: block height tidak valid\n";

                return 1;
            }
        }

BerylTransaction tx;
        std::string sourceAddress;

        bool created = false;

        for (const auto& address :
             addresses)
        {
            BerylTransaction candidate;

            if (CreateTransaction(
                    walletUTXOs,
                    address,
                    to,
                    amount,
                    candidate,
                    currentHeight,
                    MIN_TRANSACTION_FEE))
            {
                tx = candidate;
                sourceAddress = address;
                created = true;
                break;
            }
        }

        if (!created)
        {
            std::cerr
                << "ERROR: saldo tidak cukup atau UTXO belum matang\n";

            return 1;
        }

        // ----------------------------------------------------
        // Ambil Falcon private key dari wallet.
        // ----------------------------------------------------

        FalconKey signingKey;

        if (!wallet.GetKeyForAddress(
                sourceAddress,
                signingKey))
        {
            std::cerr
                << "ERROR: Falcon key tidak ditemukan\n";

            return 1;
        }

        // ----------------------------------------------------
        // Sign transaksi secara lokal.
        // ----------------------------------------------------

        if (!SignTransaction(
                tx,
                signingKey))
        {
            std::cerr
                << "ERROR: gagal menandatangani transaksi\n";

            return 1;
        }

        // ----------------------------------------------------
        // Serialize transaksi.
        // ----------------------------------------------------

        const std::string serialized =
            SerializeBerylTransaction(tx);

        static const char* hex =
            "0123456789abcdef";

        std::string txHex;

        txHex.reserve(
            serialized.size() * 2
        );

        for (unsigned char c :
             serialized)
        {
            txHex.push_back(
                hex[c >> 4]
            );

            txHex.push_back(
                hex[c & 0x0f]
            );
        }

        // ----------------------------------------------------
        // Submit transaksi ke node.
        // ----------------------------------------------------

        std::string response;

        if (!CallRPC(
                "submittransaction " + txHex,
                response))
        {
            std::cerr
                << "ERROR: node/RPC tidak tersedia\n";

            return 1;
        }

        if (response.rfind(
                "ERROR:",
                0) == 0)
        {
            std::cerr
                << response;

            return 1;
        }

        std::cout
            << "BERYL SEND\n"
            << "WALLET: "
            << walletName
            << "\n"
            << "FROM: "
            << sourceAddress
            << "\n"
            << "TO: "
            << to
            << "\n"
            << "AMOUNT: "
            << amountText
            << " BER\n"
            << response;

        return 0;
    }


    // ========================================================
    // SEED
    // ========================================================

    if (command == "seed")
    {
        std::cout
            << wallet.GetSeedPhrase()
            << "\n";

        return 0;
    }

    // ========================================================
    // BALANCE
    // ========================================================

    if (command == "balance")
    {
        uint64_t totalBalance = 0;

        const auto addresses =
            wallet.GetAddresses();

        for (const auto& address :
             addresses)
        {
            std::string response;

            if (!CallRPC(
                    "getaddressbalance " +
                    address,
                    response))
            {
                std::cerr
                    << "ERROR: node/RPC tidak tersedia\n";

                return 1;
            }

            if (response.rfind(
                    "ERROR:",
                    0) == 0)
            {
                std::cerr
                    << response;

                return 1;
            }

            const std::string balance =
                GetValue(
                    response,
                    "balance");

            if (balance.empty())
            {
                std::cerr
                    << "ERROR: response saldo tidak valid\n";

                return 1;
            }

            try
            {
                const size_t space =
                    balance.find(' ');

                const std::string amount =
                    space == std::string::npos
                    ? balance
                    : balance.substr(
                        0,
                        space);

                // Parse BER langsung ke smallest unit.
// Tidak menggunakan floating point.
                const size_t dot =
                    amount.find('.');

                uint64_t whole = 0;
                uint64_t fraction = 0;

                if (dot == std::string::npos)
                {
                    whole = std::stoull(amount);
                }
                else
                {
                    whole = std::stoull(
                        amount.substr(0, dot)
                    );

                    std::string frac =
                        amount.substr(dot + 1);

                    if (frac.size() > 8)
                    {
                        throw std::runtime_error(
                            "terlalu banyak desimal"
                        );
                    }

                    while (frac.size() < 8)
                        frac.push_back('0');

                    fraction = std::stoull(frac);
                }

                if (whole >
                    (UINT64_MAX - fraction) / 100000000ULL)
                {
                    throw std::runtime_error(
                        "saldo overflow"
                    );
                }

                totalBalance +=
                    whole * 100000000ULL +
                    fraction;
            }

            catch (...)
            {
                std::cerr
                    << "ERROR: saldo tidak valid\n";

                return 1;
            }
        }

        const uint64_t whole =
            totalBalance / 100000000ULL;

        const uint64_t fraction =
            totalBalance % 100000000ULL;

        std::cout
            << "💎 BERYL WALLET\n"
            << "WALLET: "
            << walletName
            << "\n"
            << "ADDRESSES: "
            << addresses.size()
            << "\n"
            << "BALANCE: "
            << whole
            << ".";

        std::cout.width(8);
        std::cout.fill('0');
        std::cout
            << fraction
            << " BER\n";

        return 0;
    }

    // ========================================================
    // INFO
    // ========================================================

    if (command == "info")
    {
        const auto addresses =
            wallet.GetAddresses();

        std::cout
            << "BERYL WALLET INFO\n"
            << "WALLET: "
            << walletName
            << "\n"
            << "WALLET FILE: "
            << walletFile
            << "\n"
            << "ADDRESS: "
            << wallet.GetAddress()
            << "\n"
            << "PUBLIC KEY: "
            << wallet.GetPublicKeyHex()
            << "\n"
            << "ADDRESSES: "
            << addresses.size()
            << "\n";

        return 0;
    }

    // ========================================================
    // NODE CONNECTION TEST
    // ========================================================

    if (command == "getblockcount")
    {
        std::string height;

        if (!GetNodeBlockCount(height))
        {
            std::cerr
                << "ERROR: Beryl node/RPC tidak tersedia\n";

            return 1;
        }

        std::cout
            << height;

        return 0;
    }

    PrintUsage();

    return 1;
}
