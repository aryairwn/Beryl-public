#include "chain.h"
#include "utxomanager.h"
#include "blockhash.h"
#include "coinbase.h"
#include "consensus.h"
#include "merkle.h"

#include <iostream>
#include <ctime>

// ============================================================
// Mine test block dengan difficulty 1.
// ============================================================

static bool MineTestBlock(BerylBlock& block)
{
    for (uint32_t nonce = 0;; ++nonce)
    {
        block.header.nonce = nonce;

        block.hash =
            GetBlockHash(block.header);

        if (!block.hash.empty() &&
            block.hash[0] == '0')
        {
            return true;
        }
    }
}

// ============================================================
// Buat block test berikutnya.
// ============================================================

static BerylBlock CreateTestBlock(
    const BerylChain& chain,
    const std::string& miner
)
{
    BerylBlock block;

    block.header.version = 1;

    block.header.previousHash =
        chain.GetLastHash();

    block.header.height =
        chain.GetHeight() + 1;

    block.header.timestamp =
        static_cast<uint64_t>(time(nullptr));

    block.header.difficulty = 1;
    block.header.nonce = 0;

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            block.header.height
        );

    block.transactions.push_back(
        CreateCoinbaseTransaction(
            miner,
            subsidy
        )
    );

    block.header.merkleRoot =
        CalculateMerkleRoot(
            block.transactions
        );

    return block;
}

int main()
{
    std::cout
        << "=== BERYL CHAIN TEST ===\n";

    // ========================================================
    // 1. Chain + UTXO Manager
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;

    chain.SetUTXOManager(
        &utxoManager
    );

    // ========================================================
    // 2. Block pertama
    // ========================================================

    BerylBlock block1 =
        CreateTestBlock(
            chain,
            "berMINER1"
        );

    if (!MineTestBlock(block1))
    {
        std::cout
            << "BLOCK 1 MINING FAIL\n";
        return 1;
    }

    if (!chain.AddBlock(block1))
    {
        std::cout
            << "BLOCK 1 ADD FAIL\n";
        return 1;
    }

    std::cout
        << "BLOCK 1 ACCEPTED OK\n";

    // ========================================================
    // 3. Height
    // ========================================================

    if (chain.GetHeight() != 1)
    {
        std::cout
            << "HEIGHT 1 TEST FAIL\n";
        return 1;
    }

    std::cout
        << "HEIGHT 1 OK\n";

    // ========================================================
    // 4. Coinbase masuk UTXO
    // ========================================================

    const uint64_t expectedBalance =
        BerylConsensus::GetBlockSubsidy(1);

    const uint64_t balance =
        utxoManager.GetBalance(
            "berMINER1"
        );

    if (balance != expectedBalance)
    {
        std::cout
            << "BLOCK 1 UTXO FAIL\n"
            << "EXPECTED: "
            << expectedBalance
            << "\n"
            << "ACTUAL: "
            << balance
            << "\n";

        return 1;
    }

    std::cout
        << "BLOCK 1 UTXO OK\n";

    // ========================================================
    // 5. Block kedua
    // ========================================================

    BerylBlock block2 =
        CreateTestBlock(
            chain,
            "berMINER2"
        );

    if (!MineTestBlock(block2))
    {
        std::cout
            << "BLOCK 2 MINING FAIL\n";
        return 1;
    }

    if (!chain.AddBlock(block2))
    {
        std::cout
            << "BLOCK 2 ADD FAIL\n";
        return 1;
    }

    std::cout
        << "BLOCK 2 ACCEPTED OK\n";

    if (chain.GetHeight() != 2)
    {
        std::cout
            << "HEIGHT 2 TEST FAIL\n";
        return 1;
    }

    std::cout
        << "HEIGHT 2 OK\n";

    // ========================================================
    // 6. Merkle root palsu
    // ========================================================

    BerylBlock badMerkle =
        CreateTestBlock(
            chain,
            "berATTACKER"
        );

    badMerkle.header.merkleRoot =
        "deadbeef";

    if (!MineTestBlock(badMerkle))
    {
        std::cout
            << "BAD MERKLE MINING FAIL\n";
        return 1;
    }

    const int heightBeforeBad =
        chain.GetHeight();

    const uint64_t balanceBeforeBad =
        utxoManager.GetBalance(
            "berATTACKER"
        );

    if (chain.AddBlock(badMerkle))
    {
        std::cout
            << "BAD MERKLE ACCEPTED -- FAIL\n";
        return 1;
    }

    std::cout
        << "BAD MERKLE REJECTED OK\n";

    // ========================================================
    // 7. State tidak boleh berubah
    // ========================================================

    if (chain.GetHeight() != heightBeforeBad)
    {
        std::cout
            << "INVALID BLOCK CHANGED CHAIN -- FAIL\n";
        return 1;
    }

    if (utxoManager.GetBalance(
            "berATTACKER") != balanceBeforeBad)
    {
        std::cout
            << "INVALID BLOCK CHANGED UTXO -- FAIL\n";
        return 1;
    }

    std::cout
        << "INVALID BLOCK STATE UNCHANGED OK\n";

    // ========================================================
    // 8. Previous hash palsu
    // ========================================================

    BerylBlock badPrevious =
        CreateTestBlock(
            chain,
            "berATTACKER2"
        );

    badPrevious.header.previousHash =
        "wrong-previous-hash";

    if (!MineTestBlock(badPrevious))
    {
        std::cout
            << "BAD PREVIOUS MINING FAIL\n";
        return 1;
    }

    if (chain.AddBlock(badPrevious))
    {
        std::cout
            << "BAD PREVIOUS ACCEPTED -- FAIL\n";
        return 1;
    }

    std::cout
        << "BAD PREVIOUS REJECTED OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL BERYL CHAIN TESTS PASSED\n";

    return 0;
}
