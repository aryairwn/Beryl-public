#include "chain.h"
#include "genesis.h"
#include "miner.h"
#include "utxomanager.h"

#include <iostream>

int main()
{
    std::cout
        << "=== BERYL GENESIS VALIDATION TEST ===\n";

    // ========================================================
    // 1. Chain + UTXO manager asli
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;

    chain.SetUTXOManager(
        &utxoManager
    );

    // ========================================================
    // 2. Buat Genesis asli Beryl
    // ========================================================

    BerylBlock genesis =
        CreateGenesisBlock();

    std::cout
        << "GENESIS CREATED OK\n";

    // ========================================================
    // 3. Cek Genesis sebelum AddBlock
    // ========================================================

    if (genesis.header.height != 1)
    {
        std::cout
            << "GENESIS HEIGHT FAIL\n";

        return 1;
    }

    std::cout
        << "HEIGHT = 1 OK\n";

    if (genesis.header.previousHash != "0")
    {
        std::cout
            << "GENESIS PREVIOUS HASH FAIL\n";

        return 1;
    }

    std::cout
        << "PREVIOUS HASH OK\n";

    if (genesis.transactions.size() != 1)
    {
        std::cout
            << "GENESIS TRANSACTION COUNT FAIL\n";

        return 1;
    }

    std::cout
        << "GENESIS COINBASE OK\n";

    if (genesis.transactions[0].vout.empty())
    {
        std::cout
            << "GENESIS COINBASE OUTPUT FAIL\n";

        return 1;
    }

    const uint64_t genesisReward =
        genesis.transactions[0].vout[0].amount;

    if (genesisReward != 4000000000ULL)
    {
        std::cout
            << "GENESIS REWARD FAIL\n"
            << "EXPECTED: 4000000000 units\n"
            << "ACTUAL: "
            << genesisReward
            << "\n";

        return 1;
    }

    std::cout
        << "COINBASE = 40 BER OK\n";

    // ========================================================
    // 4. Genesis harus diterima AddBlock()
    // ========================================================

    if (!chain.AddBlock(genesis))
    {
        std::cout
            << "GENESIS ADD FAIL\n";

        return 1;
    }

    std::cout
        << "GENESIS VALID + ADDED OK\n";

    // ========================================================
    // 5. Chain height
    // ========================================================

    if (chain.GetHeight() != 1)
    {
        std::cout
            << "CHAIN HEIGHT 1 FAIL\n";

        return 1;
    }

    std::cout
        << "CHAIN HEIGHT = 1 OK\n";

    // ========================================================
    // 6. Genesis UTXO
    // ========================================================

    if (utxoManager.GetBalance("berGENESIS")
        != 4000000000ULL)
    {
        std::cout
            << "GENESIS UTXO FAIL\n"
            << "EXPECTED: 4000000000 units\n"
            << "ACTUAL: "
            << utxoManager.GetBalance("berGENESIS")
            << "\n";

        return 1;
    }

    std::cout
        << "GENESIS UTXO = 40 BER OK\n";

    // ========================================================
    // 7. Mine BLOCK 2 menggunakan miner asli
    // ========================================================

    BerylBlock block2;

    if (!MineBlock(
            block2,
            chain,
            "berMINER"))
    {
        std::cout
            << "BLOCK 2 MINING FAIL\n";

        return 1;
    }

    std::cout
        << "BLOCK 2 MINED OK\n";

    // ========================================================
    // 8. AddBlock block 2
    // ========================================================

    if (!chain.AddBlock(block2))
    {
        std::cout
            << "BLOCK 2 ADD FAIL\n";

        return 1;
    }

    std::cout
        << "BLOCK 2 ACCEPTED OK\n";

    // ========================================================
    // 9. Height harus 2
    // ========================================================

    if (chain.GetHeight() != 2)
    {
        std::cout
            << "CHAIN HEIGHT 2 FAIL\n";

        return 1;
    }

    std::cout
        << "CHAIN HEIGHT = 2 OK\n";

    // ========================================================
    // 10. UTXO miner
    // ========================================================

    if (utxoManager.GetBalance("berMINER")
        != 4000000000ULL)
    {
        std::cout
            << "MINER UTXO FAIL\n"
            << "EXPECTED: 4000000000 units\n"
            << "ACTUAL: "
            << utxoManager.GetBalance("berMINER")
            << "\n";

        return 1;
    }

    std::cout
        << "MINER UTXO = 40 BER OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL BERYL GENESIS TESTS PASSED\n";

    return 0;
}
