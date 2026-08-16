// © Arya Irawan — 11 August 2026

#include "chain.h"
#include "genesis.h"
#include "storage.h"
#include "rpc.h"
#include "p2p.h"
#include "utxomanager.h"
#include "mempool.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// ============================================================
// BERYL EXPLORER PROCESS
// ============================================================

static volatile std::sig_atomic_t g_shutdown_requested = 0;
static pid_t g_explorer_pid = -1;

static void HandleShutdownSignal(int)
{
    g_shutdown_requested = 1;
}

static bool StartExplorer(const std::filesystem::path& executablePath)
{
    const std::filesystem::path baseDir = executablePath.parent_path();
    const std::filesystem::path explorerPath = baseDir / "explorer.py";
    const std::filesystem::path cliPath = baseDir / "beryl-cli";

    if (!std::filesystem::exists(explorerPath))
    {
        std::cerr << "WARNING: explorer.py tidak ditemukan: "
                  << explorerPath << "\n";
        return false;
    }

    if (!std::filesystem::exists(cliPath))
    {
        std::cerr << "WARNING: beryl-cli tidak ditemukan: "
                  << cliPath << "\n";
        return false;
    }

    const pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "WARNING: gagal membuat proses Explorer\n";
        return false;
    }

    if (pid == 0)
    {
        const std::string explorer = explorerPath.string();
        const std::string cli = cliPath.string();

        execlp(
            "python3",
            "python3",
            explorer.c_str(),
            "--cli",
            cli.c_str(),
            "--port",
            "8080",
            static_cast<char*>(nullptr)
        );

        // Hanya tercapai jika exec gagal.
        std::cerr << "WARNING: gagal menjalankan Beryl Explorer\n";
        _exit(127);
    }

    g_explorer_pid = pid;

    std::cout << " Beryl Explorer : http://127.0.0.1:8080\n";

    return true;
}

static void StopExplorer()
{
    if (g_explorer_pid <= 0)
        return;

    // Kirim SIGTERM terlebih dahulu.
    if (kill(g_explorer_pid, SIGTERM) == 0)
    {
        int status = 0;
        waitpid(g_explorer_pid, &status, 0);
    }
    else
    {
        // Pastikan child tidak menjadi zombie.
        int status = 0;
        waitpid(g_explorer_pid, &status, WNOHANG);
    }

    g_explorer_pid = -1;
}

static void CheckExplorer(
    const std::filesystem::path& executablePath
)
{
    if (g_explorer_pid <= 0)
    {
        if (!g_shutdown_requested)
        {
            std::cerr
                << "WARNING: Beryl Explorer tidak aktif. "
                << "Menjalankan kembali...\n";

            StartExplorer(executablePath);
        }

        return;
    }

    int status = 0;

    const pid_t result = waitpid(
        g_explorer_pid,
        &status,
        WNOHANG
    );

    if (result == g_explorer_pid)
    {
        g_explorer_pid = -1;

        if (!g_shutdown_requested)
        {
            std::cerr
                << "WARNING: Beryl Explorer berhenti. "
                << "Menjalankan kembali...\n";

            StartExplorer(executablePath);
        }
    }
}

int main(int argc, char* argv[])
{
    std::signal(SIGINT, HandleShutdownSignal);
    std::signal(SIGTERM, HandleShutdownSignal);

    std::cout << "Beryl Full Node Daemon v1.0\n";

    // ========================================================
    // DATA DIRECTORY
    // ========================================================

    std::string dataDir = "../data";

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--datadir")
        {
            if (i + 1 >= argc)
            {
                std::cerr
                    << "ERROR: --datadir membutuhkan path\n";
                return 1;
            }

            dataDir = argv[++i];
        }
        else if (arg == "--help")
        {
            std::cout
                << "Beryl Full Node Daemon v1.0\n"
                << "Usage:\n"
                << "  beryld [--datadir <path>]\n";
            return 0;
        }
        else
        {
            std::cerr
                << "ERROR: unknown argument: "
                << arg << "\n";
            return 1;
        }
    }

    // ========================================================
    // NODE DATA
    // ========================================================

    std::filesystem::create_directories(dataDir);

    const std::string blockchainFile =
        dataDir + "/blockchain.dat";

    // ========================================================
    // CHAIN STATE
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;
    Mempool mempool;

    chain.SetUTXOManager(
        &utxoManager
    );

    // ========================================================
    // BLOCKCHAIN PERSISTENCE
    // ========================================================

    if (std::filesystem::exists(blockchainFile))
    {
        std::cout
            << "Blockchain file ditemukan.\n"
            << "Loading blockchain...\n";

        if (!LoadBlockchain(
                chain,
                utxoManager,
                blockchainFile))
        {
            std::cerr
                << "ERROR: Blockchain file gagal dimuat.\n"
                << "Node dihentikan untuk mencegah chain rusak.\n";

            return 1;
        }

        std::cout
            << "BLOCKCHAIN LOADED\n"
            << "HEIGHT: "
            << chain.GetHeight()
            << "\n";

        std::cout
            << "LAST HASH: "
            << chain.GetLastHash()
            << "\n";

        std::cout
            << "UTXO REBUILT\n";
    }
    else
    {
        // ====================================================
        // CREATE REAL BERYL GENESIS
        // ====================================================

        std::cout
            << "Blockchain belum ada.\n"
            << "Creating real Beryl Genesis...\n";

        BerylBlock genesis =
            CreateGenesisBlock();

        if (!chain.AddBlock(genesis))
        {
            std::cerr
                << "GENESIS ADD FAILED\n";
            return 1;
        }

        std::cout
            << "GENESIS CREATED\n";

        std::cout
            << "GENESIS HASH: "
            << genesis.hash
            << "\n";

        std::cout
            << "CHAIN HEIGHT: "
            << chain.GetHeight()
            << "\n";

        if (!SaveBlockchain(
                chain,
                blockchainFile))
        {
            std::cerr
                << "GENESIS SAVE FAILED\n";
            return 1;
        }

        std::cout
            << "GENESIS SAVED\n";
    }

    // ========================================================
    // RPC
    // ========================================================
    //
    // RPC node hanya menyediakan fungsi node publik.
    //
    // Tidak ada:
    // - wallet
    // - seed phrase
    // - private key
    // - signing
    // - balance wallet
    // - wallet restore
    //
    // Miner standalone menggunakan RPC:
    // - getblocktemplate
    // - submitblock
    //
    // ========================================================

    StartRPC(
        chain,
        utxoManager,
        mempool,
        blockchainFile
    );

    // ========================================================
    // P2P NETWORK
    // ========================================================

    StartP2P(
        chain,
        utxoManager,
        mempool
    );

    // ========================================================
    // BERYL BLOCK EXPLORER
    // ========================================================

    const std::filesystem::path executablePath =
        std::filesystem::absolute(argv[0]);

    StartExplorer(executablePath);

    // ========================================================
    // FULL NODE MODE
    // ========================================================

    std::cout
        << "==================================================\n"
        << " BERYL FULL NODE READY\n"
        << "--------------------------------------------------\n"
        << " wallet : standalone\n"
        << " miner  : standalone\n"
        << " node   : blockchain + UTXO + mempool + P2P + RPC\n"
        << " PoW    : external miner\n"
        << "==================================================\n";

    // Node tetap hidup sambil memantau shutdown
    // dan status proses Explorer.
    while (!g_shutdown_requested)
    {
        CheckExplorer(executablePath);

        std::this_thread::sleep_for(
            std::chrono::seconds(1)
        );
    }

    std::cout << "\nShutting down Beryl Full Node...\n";

    StopExplorer();

    std::cout << "Beryl Full Node stopped.\n";

    return 0;
}
