#include "storage.h"
#include "miner.h"
#include "chain.h"
#include "utxomanager.h"
#include "genesis.h"

#include <iostream>
#include <cstdio>

int main()
{
    std::cout << "=== BERYL PERSISTENCE TEST ===\n";

    const std::string filename = "test-blockchain.dat";

    // ========================================================
    // 1. Chain pertama
    // ========================================================

    BerylChain chain1;
    UTXOManager utxo1;

    chain1.SetUTXOManager(&utxo1);

    // ========================================================
    // 2. REAL GENESIS
    // ========================================================

    BerylBlock genesis = CreateGenesisBlock();

    if (!chain1.AddBlock(genesis))
    {
        std::cout << "GENESIS ADD FAIL\n";
        return 1;
    }

    std::cout << "GENESIS ADD OK\n";

    if (chain1.GetHeight() != 1)
    {
        std::cout << "GENESIS HEIGHT FAIL\n";
        return 1;
    }

    std::cout << "GENESIS HEIGHT = 1 OK\n";

    // ========================================================
    // 3. Mine BLOCK 2
    // ========================================================

    BerylBlock block1;

    if (!MineBlock(
            block1,
            chain1,
            "berALICE"))
    {
        std::cout << "BLOCK 2 MINING FAIL\n";
        return 1;
    }

    if (!chain1.AddBlock(block1))
    {
        std::cout << "BLOCK 2 ADD FAIL\n";
        return 1;
    }

    std::cout << "BLOCK 2 ADD OK\n";

    // ========================================================
    // 4. Mine BLOCK 3
    // ========================================================

    BerylBlock block2;

    if (!MineBlock(
            block2,
            chain1,
            "berBOB"))
    {
        std::cout << "BLOCK 3 MINING FAIL\n";
        return 1;
    }

    if (!chain1.AddBlock(block2))
    {
        std::cout << "BLOCK 3 ADD FAIL\n";
        return 1;
    }

    std::cout << "BLOCK 3 ADD OK\n";

    // ========================================================
    // 5. CHECK BEFORE SAVE
    // ========================================================

    if (chain1.GetHeight() != 3)
    {
        std::cout << "HEIGHT BEFORE SAVE FAIL\n";
        return 1;
    }

    std::cout << "HEIGHT BEFORE SAVE = 3 OK\n";

    const std::string hashBefore =
        chain1.GetLastHash();

    if (hashBefore != block2.hash)
    {
        std::cout << "HASH BEFORE SAVE FAIL\n";
        return 1;
    }

    std::cout << "HASH BEFORE SAVE OK\n";

    // ========================================================
    // 6. CHECK UTXO
    // ========================================================

    const uint64_t aliceBefore =
        utxo1.GetBalance("berALICE");

    const uint64_t bobBefore =
        utxo1.GetBalance("berBOB");

    if (aliceBefore != 4000000000ULL)
    {
        std::cout << "ALICE BALANCE FAIL\n";
        std::cout << "ACTUAL: "
                  << aliceBefore
                  << "\n";
        return 1;
    }

    if (bobBefore != 4000000000ULL)
    {
        std::cout << "BOB BALANCE FAIL\n";
        std::cout << "ACTUAL: "
                  << bobBefore
                  << "\n";
        return 1;
    }

    std::cout << "UTXO BEFORE SAVE OK\n";

    // ========================================================
    // 7. SAVE
    // ========================================================

    if (!SaveBlockchain(
            chain1,
            filename))
    {
        std::cout << "SAVE FAIL\n";
        return 1;
    }

    std::cout << "SAVE OK\n";

    // ========================================================
    // 8. SIMULATE RESTART
    // ========================================================

    BerylChain chain2;
    UTXOManager utxo2;

    if (chain2.GetHeight() != 0)
    {
        std::cout << "NEW CHAIN NOT EMPTY\n";
        return 1;
    }

    std::cout << "NEW CHAIN EMPTY OK\n";

    // ========================================================
    // 9. LOAD
    // ========================================================

    if (!LoadBlockchain(
            chain2,
            utxo2,
            filename))
    {
        std::cout << "LOAD FAIL\n";
        return 1;
    }

    std::cout << "LOAD OK\n";

    // ========================================================
    // 10. HEIGHT
    // ========================================================

    if (chain2.GetHeight() != 3)
    {
        std::cout << "HEIGHT AFTER LOAD FAIL\n";
        std::cout << "EXPECTED: 3\n";
        std::cout << "ACTUAL: "
                  << chain2.GetHeight()
                  << "\n";
        return 1;
    }

    std::cout << "HEIGHT AFTER LOAD = 3 OK\n";

    // ========================================================
    // 11. HASH
    // ========================================================

    if (chain2.GetLastHash() != hashBefore)
    {
        std::cout << "HASH AFTER LOAD FAIL\n";
        return 1;
    }

    std::cout << "HASH AFTER LOAD OK\n";

    // ========================================================
    // 12. UTXO REBUILD
    // ========================================================

    const uint64_t aliceAfter =
        utxo2.GetBalance("berALICE");

    const uint64_t bobAfter =
        utxo2.GetBalance("berBOB");

    if (aliceAfter != aliceBefore)
    {
        std::cout << "ALICE BALANCE AFTER LOAD FAIL\n";
        return 1;
    }

    if (bobAfter != bobBefore)
    {
        std::cout << "BOB BALANCE AFTER LOAD FAIL\n";
        return 1;
    }

    std::cout << "UTXO REBUILD OK\n";

    // ========================================================
    // 13. MANAGER RECONNECT
    // ========================================================

    if (chain2.GetUTXOManager() != &utxo2)
    {
        std::cout << "UTXO MANAGER RECONNECT FAIL\n";
        return 1;
    }

    std::cout << "UTXO MANAGER RECONNECT OK\n";

    // ========================================================
    // 14. CLEANUP
    // ========================================================

    std::remove(filename.c_str());

    std::cout << "TEST FILE REMOVED OK\n";

    std::cout
        << "\nALL BERYL PERSISTENCE TESTS PASSED\n";

    return 0;
}
