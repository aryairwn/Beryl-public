// © Arya Irawan — 10 August 2026

#include "miner.h"
#include "blockvalidation.h"
#include "blockhash.h"
#include "consensus.h"
#include "difficulty.h"

#include <cstdint>

static bool CheckMiningTarget(
    const std::string& hash,
    uint64_t target
)
{
    if (target == 0 || hash.size() < 16)
        return false;

    uint64_t hashValue = 0;

    for (int i = 0; i < 16; ++i)
    {
        const char c = hash[i];

        uint64_t nibble;

        if (c >= '0' && c <= '9')
            nibble =
                static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f')
            nibble =
                static_cast<uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            nibble =
                static_cast<uint64_t>(c - 'A' + 10);
        else
            return false;

        hashValue =
            (hashValue << 4) | nibble;
    }

    return hashValue <= target;
}


#include "coinbase.h"
#include "merkle.h"
#include "mempool.h"
#include "utxomanager.h"
#include "utxoroot.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <algorithm>

static std::atomic<uint64_t> g_hashAttempts{0};
static std::atomic<int> g_activeMiningWorkers{0};

static std::mutex g_hashrateMutex;

static uint64_t g_lastHashrateAttempts = 0;
static uint64_t g_lastHashrateTime = 0;

static uint64_t GetNowMilliseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t GetMiningHashAttempts()
{
    return g_hashAttempts.load(
        std::memory_order_relaxed
    );
}

uint64_t GetMiningHashrate()
{
    std::lock_guard<std::mutex> lock(
        g_hashrateMutex
    );

    const uint64_t now =
        GetNowMilliseconds();

    const uint64_t attempts =
        g_hashAttempts.load(
            std::memory_order_relaxed
        );

    // Inisialisasi pengukuran pertama.
    if (g_lastHashrateTime == 0)
    {
        g_lastHashrateTime = now;
        g_lastHashrateAttempts = attempts;
        return 0;
    }

    const uint64_t elapsed =
        now > g_lastHashrateTime
            ? now - g_lastHashrateTime
            : 0;

    const uint64_t deltaAttempts =
        attempts >= g_lastHashrateAttempts
            ? attempts - g_lastHashrateAttempts
            : 0;

    g_lastHashrateAttempts = attempts;
    g_lastHashrateTime = now;

    if (elapsed == 0)
        return 0;

    return (deltaAttempts * 1000ULL) / elapsed;
}

static int ReadExternalMinerWorkers()
{
    std::ifstream file("miner.status");

    if (!file)
        return 0;

    std::string line;

    bool running = false;
    int workers = 0;

    while (std::getline(file, line))
    {
        if (line == "running=1")
        {
            running = true;
            continue;
        }

        const std::string prefix = "workers=";

        if (line.rfind(prefix, 0) == 0)
        {
            try
            {
                workers =
                    std::stoi(
                        line.substr(prefix.size())
                    );
            }
            catch (...)
            {
                workers = 0;
            }
        }
    }

    if (!running || workers < 1)
        return 0;

    return workers;
}

int GetMiningWorkers()
{
    return ReadExternalMinerWorkers();
}

static std::string FormatBER(uint64_t amount)
{
    const uint64_t COIN = 100000000ULL;

    std::ostringstream ss;

    ss << (amount / COIN)
       << '.'
       << std::setw(8)
       << std::setfill('0')
       << (amount % COIN)
       << " BER";

    return ss.str();
}


bool CreateBlockTemplate(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
)
{
    // --------------------------------------------------------
    // 1. UTXO manager wajib tersedia.
    // --------------------------------------------------------

    const UTXOManager* utxoManager =
        chain.GetUTXOManager();

    if (utxoManager == nullptr)
        return false;

    // --------------------------------------------------------
    // 2. Tentukan height block berikutnya.
    // --------------------------------------------------------

    const int height =
        chain.GetHeight() + 1;

    block.header.version = 1;

    block.header.height =
        height;

    block.header.previousHash =
        chain.GetLastHash();

    block.header.timestamp =
        static_cast<uint64_t>(time(nullptr));

    block.header.difficulty =
        CalculateDifficulty(chain);

    block.header.nonce = 0;

    // --------------------------------------------------------
    // 3. Subsidy berdasarkan height.
    // --------------------------------------------------------

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            height
        );

    // --------------------------------------------------------
    // 4. Ambil transaksi dari mempool.
    // --------------------------------------------------------

    block.transactions.clear();

    const auto& mempoolTransactions =
        mempool.GetTransactions();

    // --------------------------------------------------------
    // 4. Pilih transaksi sampai batas MAX_BLOCK_SIZE.
    //
    // Gunakan UTXO candidate secara sequential agar miner
    // mengikuti aturan consensus ValidateBlock().
    //
    // TX yang sudah dipilih akan diterapkan ke candidate UTXO.
    // Dengan demikian TX berikutnya dapat menggunakan UTXO
    // yang dihasilkan TX sebelumnya dalam block yang sama.
    // --------------------------------------------------------
    UTXOManager selectionUTXO = *utxoManager;


    // Candidate ContractState berjalan paralel dengan
    // candidate UTXO selama pemilihan transaksi.
    beryl::contract::ContractStateManager selectionContractState =
        chain.GetContractStateManager();

    uint64_t totalFees = 0;

    for (const auto& tx : mempoolTransactions)
    {
        // ----------------------------------------------------
        // Validasi terhadap state UTXO candidate TERKINI.
        // ----------------------------------------------------
        if (!ValidateTransaction(
                selectionUTXO.GetUTXOSet(),
                tx,
                height
            ))
        {
            continue;
        }

        // ----------------------------------------------------
        // Hitung fee terhadap state yang sama.
        // ----------------------------------------------------
        const uint64_t fee =
            CalculateTransactionFee(
                selectionUTXO.GetUTXOSet(),
                tx
            );

        if (fee < MIN_TRANSACTION_FEE)
            continue;

        if (UINT64_MAX - totalFees < fee)
            continue;

        const uint64_t candidateFees =
            totalFees + fee;

        if (UINT64_MAX - subsidy < candidateFees)
            continue;

        const uint64_t candidateReward =
            subsidy + candidateFees;

        // ----------------------------------------------------
        // Buat coinbase sementara dengan reward terbaru.
        // ----------------------------------------------------
        BerylTransaction candidateCoinbase =
            CreateCoinbaseTransaction(
                minerAddress,
                candidateReward,
                "BERYL-COINBASE-HEIGHT-" +
                std::to_string(height)
            );

        // ----------------------------------------------------
        // Buat candidate block lengkap:
        //
        // coinbase
        // + TX yang sudah dipilih
        // + TX baru yang sedang diuji
        // ----------------------------------------------------
        BerylBlock candidateBlock = block;

        candidateBlock.transactions.clear();

        candidateBlock.transactions.push_back(
            candidateCoinbase
        );

        for (const auto& selectedTx : block.transactions)
        {
            candidateBlock.transactions.push_back(
                selectedTx
            );
        }

        candidateBlock.transactions.push_back(tx);

        // ----------------------------------------------------
        // Gunakan ukuran canonical root/hash final.
        // Root sebenarnya dihitung setelah semua TX terpilih.
        // Untuk selection, gunakan panjang canonical 64 hex.
        // ----------------------------------------------------
        candidateBlock.header.merkleRoot =
            std::string(64, '0');

        candidateBlock.header.utxoRoot =
            std::string(64, '0');

        candidateBlock.hash =
            std::string(64, '0');

        const std::string serializedCandidate =
            SerializeBerylBlock(candidateBlock);

        if (serializedCandidate.size() >
            BerylConsensus::MAX_BLOCK_SIZE)
        {
            continue;
        }

        // ----------------------------------------------------
        // TX lolos:
        // masukkan ke block dan terapkan ke candidate UTXO.
        // ----------------------------------------------------
        if (!ApplyTransaction(
                selectionUTXO.GetUTXOSetMutable(),
                tx,
                height
            ))
        {
            continue;
        }

        
        // Contract state adalah bagian dari consensus.
        // Terapkan TX ke candidate ContractState yang sama
        // dengan state yang diperiksa ValidateBlock().
        if (!ApplyContractTransactionState(
                selectionContractState,
                tx,
                height
            ))
        {
            continue;
        }

block.transactions.push_back(tx);

        totalFees = candidateFees;
    }

    // --------------------------------------------------------
    // 5. Total fee sekarang hanya berasal dari TX terpilih.
    // --------------------------------------------------------
    // --------------------------------------------------------

    // --------------------------------------------------------
    // 6. Cegah overflow subsidy + fee.
    // --------------------------------------------------------

    if (UINT64_MAX - subsidy < totalFees)
        return false;

    const uint64_t coinbaseReward =
        subsidy + totalFees;

    // --------------------------------------------------------
    // 7. Buat coinbase.
    //
    // Coinbase harus menjadi transaksi pertama.
    // --------------------------------------------------------

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            minerAddress,
            coinbaseReward,
            "BERYL-COINBASE-HEIGHT-" +
            std::to_string(height)
        );

    block.transactions.insert(
        block.transactions.begin(),
        coinbase
    );

    block.reward =
        coinbaseReward;

    // --------------------------------------------------------
    // 8. Merkle root setelah seluruh transaksi final.
    // --------------------------------------------------------

    block.header.merkleRoot =
        CalculateMerkleRoot(
            block.transactions
        );

// --------------------------------------------------------
// 8b. Hitung deterministic UTXO root.
//      Root harus merepresentasikan UTXO SET
//      setelah candidate block diterapkan.
// --------------------------------------------------------
UTXOManager candidateUTXO = *utxoManager;

candidateUTXO.ProcessBlock(block);

block.header.utxoRoot =
    CalculateUTXORoot(candidateUTXO);

if (block.header.utxoRoot.empty())
{
    std::cerr
        << "MINING ERROR: gagal menghitung UTXO root\n";
    return false;
}

    
    // --------------------------------------------------------
    // CONTRACT STATE ROOT
    //
    // Candidate ContractState sudah berisi seluruh TX
    // contract yang dipilih secara sequential.
    // --------------------------------------------------------

    block.header.contractRoot =
        selectionContractState.CalculateRoot();

    if (block.header.contractRoot.empty())
    {
        std::cerr
            << "MINING ERROR: gagal menghitung CONTRACT root\n";
        return false;
    }

// --------------------------------------------------------
    // 8c. Validasi ukuran block FINAL.
    //
    // Merkle root dan UTXO root sudah final sehingga ukuran
    // serialized block sekarang adalah ukuran sebenarnya.
    // --------------------------------------------------------
    const std::string serializedFinalBlock =
        SerializeBerylBlock(block);

    if (serializedFinalBlock.size() >
        BerylConsensus::MAX_BLOCK_SIZE)
    {
        std::cerr
            << "MINING ERROR: final block melebihi MAX_BLOCK_SIZE\n";
        return false;
    }

    // ========================================================
    // BLOCK TEMPLATE READY
    // ========================================================
    // Belum melakukan Proof of Work.
    //
    // Nonce masih 0 dan hash belum merupakan PoW result.
    // ========================================================
    return true;
}

bool MineBlock(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
)
{
    // --------------------------------------------------------
    // 1. UTXO manager wajib tersedia.
    // --------------------------------------------------------

    const UTXOManager* utxoManager =
        chain.GetUTXOManager();

    if (utxoManager == nullptr)
        return false;

    // --------------------------------------------------------
    // 2. Tentukan height block berikutnya.
    // --------------------------------------------------------

    const int height =
        chain.GetHeight() + 1;

    block.header.version = 1;

    block.header.height =
        height;

    block.header.previousHash =
        chain.GetLastHash();

    block.header.timestamp =
        static_cast<uint64_t>(time(nullptr));

    block.header.difficulty =
        CalculateDifficulty(chain);

    block.header.nonce = 0;

    // --------------------------------------------------------
    // 3. Subsidy berdasarkan height.
    // --------------------------------------------------------

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            height
        );

    // --------------------------------------------------------
    // 4. Ambil transaksi dari mempool.
    // --------------------------------------------------------

    block.transactions.clear();

    const auto& mempoolTransactions =
        mempool.GetTransactions();

    // --------------------------------------------------------
    // 4. Pilih transaksi sampai batas MAX_BLOCK_SIZE.
    //
    // Gunakan UTXO candidate secara sequential agar miner
    // mengikuti aturan consensus ValidateBlock().
    //
    // TX yang sudah dipilih akan diterapkan ke candidate UTXO.
    // Dengan demikian TX berikutnya dapat menggunakan UTXO
    // yang dihasilkan TX sebelumnya dalam block yang sama.
    // --------------------------------------------------------
    UTXOManager selectionUTXO = *utxoManager;


    // Candidate ContractState berjalan paralel dengan
    // candidate UTXO selama pemilihan transaksi.
    beryl::contract::ContractStateManager selectionContractState =
        chain.GetContractStateManager();

    uint64_t totalFees = 0;

    for (const auto& tx : mempoolTransactions)
    {
        // ----------------------------------------------------
        // Validasi terhadap state UTXO candidate TERKINI.
        // ----------------------------------------------------
        if (!ValidateTransaction(
                selectionUTXO.GetUTXOSet(),
                tx,
                height
            ))
        {
            continue;
        }

        // ----------------------------------------------------
        // Hitung fee terhadap state yang sama.
        // ----------------------------------------------------
        const uint64_t fee =
            CalculateTransactionFee(
                selectionUTXO.GetUTXOSet(),
                tx
            );

        if (fee < MIN_TRANSACTION_FEE)
            continue;

        if (UINT64_MAX - totalFees < fee)
            continue;

        const uint64_t candidateFees =
            totalFees + fee;

        if (UINT64_MAX - subsidy < candidateFees)
            continue;

        const uint64_t candidateReward =
            subsidy + candidateFees;

        // ----------------------------------------------------
        // Buat coinbase sementara dengan reward terbaru.
        // ----------------------------------------------------
        BerylTransaction candidateCoinbase =
            CreateCoinbaseTransaction(
                minerAddress,
                candidateReward,
                "BERYL-COINBASE-HEIGHT-" +
                std::to_string(height)
            );

        // ----------------------------------------------------
        // Buat candidate block lengkap:
        //
        // coinbase
        // + TX yang sudah dipilih
        // + TX baru yang sedang diuji
        // ----------------------------------------------------
        BerylBlock candidateBlock = block;

        candidateBlock.transactions.clear();

        candidateBlock.transactions.push_back(
            candidateCoinbase
        );

        for (const auto& selectedTx : block.transactions)
        {
            candidateBlock.transactions.push_back(
                selectedTx
            );
        }

        candidateBlock.transactions.push_back(tx);

        // ----------------------------------------------------
        // Gunakan ukuran canonical root/hash final.
        // Root sebenarnya dihitung setelah semua TX terpilih.
        // Untuk selection, gunakan panjang canonical 64 hex.
        // ----------------------------------------------------
        candidateBlock.header.merkleRoot =
            std::string(64, '0');

        candidateBlock.header.utxoRoot =
            std::string(64, '0');

        candidateBlock.hash =
            std::string(64, '0');

        const std::string serializedCandidate =
            SerializeBerylBlock(candidateBlock);

        if (serializedCandidate.size() >
            BerylConsensus::MAX_BLOCK_SIZE)
        {
            continue;
        }

        // ----------------------------------------------------
        // TX lolos:
        // masukkan ke block dan terapkan ke candidate UTXO.
        // ----------------------------------------------------
        if (!ApplyTransaction(
                selectionUTXO.GetUTXOSetMutable(),
                tx,
                height
            ))
        {
            continue;
        }

        
        // Contract state adalah bagian dari consensus.
        // Terapkan TX ke candidate ContractState yang sama
        // dengan state yang diperiksa ValidateBlock().
        if (!ApplyContractTransactionState(
                selectionContractState,
                tx,
                height
            ))
        {
            continue;
        }

block.transactions.push_back(tx);

        totalFees = candidateFees;
    }

    // --------------------------------------------------------
    // 5. Total fee sekarang hanya berasal dari TX terpilih.
    // --------------------------------------------------------
    // --------------------------------------------------------

    // --------------------------------------------------------
    // 6. Cegah overflow subsidy + fee.
    // --------------------------------------------------------

    if (UINT64_MAX - subsidy < totalFees)
        return false;

    const uint64_t coinbaseReward =
        subsidy + totalFees;

    // --------------------------------------------------------
    // 7. Buat coinbase.
    //
    // Coinbase harus menjadi transaksi pertama.
    // --------------------------------------------------------

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            minerAddress,
            coinbaseReward,
            "BERYL-COINBASE-HEIGHT-" +
            std::to_string(height)
        );

    block.transactions.insert(
        block.transactions.begin(),
        coinbase
    );

    block.reward =
        coinbaseReward;

    // --------------------------------------------------------
    // 8. Merkle root setelah seluruh transaksi final.
    // --------------------------------------------------------

    block.header.merkleRoot =
        CalculateMerkleRoot(
            block.transactions
        );

// --------------------------------------------------------
// 8b. Hitung deterministic UTXO root.
//      Root harus merepresentasikan UTXO SET
//      setelah candidate block diterapkan.
// --------------------------------------------------------
UTXOManager candidateUTXO = *utxoManager;

candidateUTXO.ProcessBlock(block);

block.header.utxoRoot =
    CalculateUTXORoot(candidateUTXO);

if (block.header.utxoRoot.empty())
{
    std::cerr
        << "MINING ERROR: gagal menghitung UTXO root\n";
    return false;
}

    
    // --------------------------------------------------------
    // CONTRACT STATE ROOT
    //
    // Candidate ContractState sudah berisi seluruh TX
    // contract yang dipilih secara sequential.
    // --------------------------------------------------------

    block.header.contractRoot =
        selectionContractState.CalculateRoot();

    if (block.header.contractRoot.empty())
    {
        std::cerr
            << "MINING ERROR: gagal menghitung CONTRACT root\n";
        return false;
    }

// --------------------------------------------------------
    // 8c. Validasi ukuran block FINAL.
    //
    // Merkle root dan UTXO root sudah final sehingga ukuran
    // serialized block sekarang adalah ukuran sebenarnya.
    // --------------------------------------------------------
    const std::string serializedFinalBlock =
        SerializeBerylBlock(block);

    if (serializedFinalBlock.size() >
        BerylConsensus::MAX_BLOCK_SIZE)
    {
        std::cerr
            << "MINING ERROR: final block melebihi MAX_BLOCK_SIZE\n";
        return false;
    }

    // 9. Proof of Work YesPower.
    // --------------------------------------------------------

    // --------------------------------------------------------
    // 9. Proof of Work YesPower - MULTI WORKER.
    //
    // Setiap worker menggunakan nonce berbeda:
    //
    // worker 0: 0, W, 2W, 3W, ...
    // worker 1: 1, W+1, 2W+1, ...
    //
    // Dengan demikian tidak ada worker yang mengulang
    // nonce worker lainnya.
    // --------------------------------------------------------

    const unsigned int workerCount = 1;

    const auto powStart =
        std::chrono::steady_clock::now();

    const uint64_t powAttemptsStart =
        g_hashAttempts.load(
            std::memory_order_relaxed
        );

    std::atomic<bool> foundBlock{false};

    std::atomic<uint32_t> foundNonce{0};

    std::string foundHash;

    std::mutex foundMutex;

    std::vector<std::thread> workers;

    workers.reserve(workerCount);

    g_activeMiningWorkers.store(
        static_cast<int>(workerCount),
        std::memory_order_release
    );

    for (unsigned int workerId = 0;
         workerId < workerCount;
         ++workerId)
    {
        workers.emplace_back(
            [
                &block,
                workerId,
                workerCount,
                &foundBlock,
                &foundNonce,
                &foundHash,
                &foundMutex
            ]()
            {
                for (
                    uint64_t nonce =
                        workerId;
                    nonce <= UINT32_MAX;
                    nonce += workerCount
                )
                {
                    if (
                        foundBlock.load(
                            std::memory_order_acquire
                        )
                    )
                    {
                        break;
                    }

                    BerylHeader header =
                        block.header;

                    header.nonce =
                        static_cast<uint32_t>(
                            nonce
                        );

                    const std::string hash =
                        GetBlockHash(header);

                    g_hashAttempts.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );

                    const bool valid =
                        CheckMiningTarget(
                            hash,
                            header.difficulty
                        );

                    if (!valid)
                        continue;

                    bool expected = false;

                    if (
                        foundBlock.compare_exchange_strong(
                            expected,
                            true,
                            std::memory_order_acq_rel
                        )
                    )
                    {
                        std::lock_guard<std::mutex>
                            lock(foundMutex);

                        foundNonce.store(
                            header.nonce,
                            std::memory_order_release
                        );

                        foundHash = hash;
                    }

                    break;
                }
            }
        );
    }

    for (auto& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }

    g_activeMiningWorkers.store(
        0,
        std::memory_order_release
    );

    const auto powEnd =
        std::chrono::steady_clock::now();

    const double powSeconds =
        std::chrono::duration<double>(
            powEnd - powStart
        ).count();

    const uint64_t powAttemptsEnd =
        g_hashAttempts.load(
            std::memory_order_relaxed
        );

    std::cout
        << "POW TIME: "
        << powSeconds
        << " s"
        << " | ATTEMPTS: "
        << (powAttemptsEnd - powAttemptsStart)
        << "\\n";

    if (!foundBlock.load(
            std::memory_order_acquire
        ))
    {
        std::cerr
            << "MINING ERROR: nonce space exhausted\\n";

        return false;
    }

    block.header.nonce =
        foundNonce.load(
            std::memory_order_acquire
        );

    {
        std::lock_guard<std::mutex>
            lock(foundMutex);

        block.hash = foundHash;
    }

    const bool valid =
        CheckMiningTarget(
            block.hash,
            block.header.difficulty
        );

    if (valid)
        {
            std::cout
                << "BLOCK FOUND\n";

            std::cout
                << "HEIGHT: "
                << height
                << "\n";

            std::cout
                << "SUBSIDY: "
                << FormatBER(subsidy)
                << "\n";

            std::cout
                << "FEES: "
                << FormatBER(totalFees)
                << "\n";

            std::cout
                << "COINBASE REWARD: "
                << FormatBER(coinbaseReward)
                << "\n";

            std::cout
                << "TRANSACTIONS: "
                << block.transactions.size()
                << "\n";

            std::cout
                << "NONCE: "
                << block.header.nonce
                << "\n";

            std::cout
                << "DIFFICULTY: "
                << block.header.difficulty
                << "\n";

            // --------------------------------------------------------
            // Block sudah selesai di-mine.
            //
            // Ukur waktu yang dibutuhkan chain.AddBlock().
            //

            const auto addBlockStart =
                std::chrono::steady_clock::now();

            if (!chain.AddBlock(block))
            {
                const auto addBlockEnd =
                    std::chrono::steady_clock::now();

                const double addBlockSeconds =
                    std::chrono::duration<double>(
                        addBlockEnd - addBlockStart
                    ).count();

                std::cerr
                    << "MINING ERROR: block ditolak chain\n"
                    << "ADD BLOCK TIME: "
                    << addBlockSeconds
                    << " s\n";

                return false;
            }

            const auto addBlockEnd =
                std::chrono::steady_clock::now();

            const double addBlockSeconds =
                std::chrono::duration<double>(
                    addBlockEnd - addBlockStart
                ).count();

            std::cout
                << "ADD BLOCK TIME: "
                << addBlockSeconds
                << " s\n";

            // Block sudah resmi masuk chain.
            //
            // Sekarang transaksi non-coinbase yang sudah masuk block
            // boleh dihapus dari mempool.
            // --------------------------------------------------------

            mempool.RemoveTransactions(
                block.transactions
            );

            std::cout
                << "BLOCK ACCEPTED BY CHAIN\n";

            std::cout
                << "MEMPOOL REMAINING: "
                << mempool.Size()
                << "\n";

            return true;
        }

    return false;

    }
