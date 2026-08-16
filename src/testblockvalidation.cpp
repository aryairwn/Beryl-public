#include "blockvalidation.h"
#include "blockhash.h"
#include "coinbase.h"
#include "consensus.h"
#include "merkle.h"

#include <iostream>
#include <ctime>

// ============================================================
// Mine a very small test block.
//
// Difficulty = 1 sehingga test cepat di Android/Termux.
// ============================================================

static bool MineTestBlock(
    BerylBlock& block
)
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

int main()
{
    std::cout
        << "=== BERYL BLOCK VALIDATION TEST ===\n";

    // ========================================================
    // 1. Chain kosong + UTXO manager
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;

    // ========================================================
    // 2. Buat block pertama.
    // ========================================================

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

    // ========================================================
    // Coinbase = subsidy year 0.
    // ========================================================

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            block.header.height
        );

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            "berMINER",
            subsidy
        );

    block.transactions.push_back(
        coinbase
    );

    // ========================================================
    // Merkle root.
    // ========================================================

    block.header.merkleRoot =
        CalculateMerkleRoot(
            block.transactions
        );

    // ========================================================
    // Mine test PoW.
    // ========================================================

    if (!MineTestBlock(block))
    {
        std::cout
            << "TEST MINING FAIL\n";
        return 1;
    }

    std::cout
        << "TEST BLOCK MINED OK\n";

    // ========================================================
    // 3. Block valid harus diterima.
    // ========================================================

    if (!ValidateBlock(
            block,
            chain,
            utxoManager))
    {
        std::cout
            << "VALID BLOCK TEST FAIL\n";
        return 1;
    }

    std::cout
        << "VALID BLOCK OK\n";

    // ========================================================
    // 4. Merkle tampering.
    // ========================================================

    BerylBlock badMerkle = block;

    badMerkle.header.merkleRoot =
        "deadbeef";

    if (ValidateBlock(
            badMerkle,
            chain,
            utxoManager))
    {
        std::cout
            << "MERKLE TAMPER TEST FAIL\n";
        return 1;
    }

    std::cout
        << "MERKLE TAMPER REJECTED OK\n";

    // ========================================================
    // 5. Previous hash tampering.
    // ========================================================

    BerylBlock badPrevious = block;

    badPrevious.header.previousHash =
        "badprevioushash";

    if (ValidateBlock(
            badPrevious,
            chain,
            utxoManager))
    {
        std::cout
            << "PREVIOUS HASH TEST FAIL\n";
        return 1;
    }

    std::cout
        << "PREVIOUS HASH REJECTED OK\n";

    // ========================================================
    // 6. Height tampering.
    // ========================================================

    BerylBlock badHeight = block;

    badHeight.header.height = 99;

    if (ValidateBlock(
            badHeight,
            chain,
            utxoManager))
    {
        std::cout
            << "HEIGHT TEST FAIL\n";
        return 1;
    }

    std::cout
        << "HEIGHT REJECTED OK\n";

    // ========================================================
    // 7. Hash tampering.
    // ========================================================

    BerylBlock badHash = block;

    badHash.hash =
        "0000000000000000000000000000000000000000000000000000000000000000";

    if (ValidateBlock(
            badHash,
            chain,
            utxoManager))
    {
        std::cout
            << "HASH TAMPER TEST FAIL\n";
        return 1;
    }

    std::cout
        << "HASH TAMPER REJECTED OK\n";

    // ========================================================
    // 8. Coinbase terlalu besar.
    // ========================================================

    BerylBlock badReward = block;

    badReward.transactions[0]
        .vout[0]
        .amount =
            subsidy + 1;

    // Merkle harus dihitung ulang karena transaksi berubah.
    badReward.header.merkleRoot =
        CalculateMerkleRoot(
            badReward.transactions
        );

    // Hash juga harus dicari ulang.
    if (!MineTestBlock(badReward))
    {
        std::cout
            << "BAD REWARD TEST MINING FAIL\n";
        return 1;
    }

    if (ValidateBlock(
            badReward,
            chain,
            utxoManager))
    {
        std::cout
            << "EXCESSIVE COINBASE TEST FAIL\n";
        return 1;
    }

    std::cout
        << "EXCESSIVE COINBASE REJECTED OK\n";

    // ========================================================
    // 9. Dua coinbase.
    // ========================================================

    BerylBlock twoCoinbase = block;

    BerylTransaction secondCoinbase =
        CreateCoinbaseTransaction(
            "berMINER2",
            1
        );

    twoCoinbase.transactions.push_back(
        secondCoinbase
    );

    twoCoinbase.header.merkleRoot =
        CalculateMerkleRoot(
            twoCoinbase.transactions
        );

    if (!MineTestBlock(twoCoinbase))
    {
        std::cout
            << "TWO COINBASE TEST MINING FAIL\n";
        return 1;
    }

    if (ValidateBlock(
            twoCoinbase,
            chain,
            utxoManager))
    {
        std::cout
            << "TWO COINBASE TEST FAIL\n";
        return 1;
    }

    std::cout
        << "TWO COINBASE REJECTED OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL BERYL BLOCK VALIDATION TESTS PASSED\n";

    return 0;
}
