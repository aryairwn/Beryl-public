#include "miner.h"
#include "mempool.h"
#include "chain.h"
#include "utxomanager.h"
#include "transaction.h"
#include "consensus.h"

#include "crypto/falcon/falcon.h"

#include <iostream>
#include <cstdint>

int main()
{
    std::cout
        << "=== BERYL MULTI TRANSACTION BLOCK TEST ===\n";

    // ========================================================
    // 1. Chain + UTXO Manager
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;

    chain.SetUTXOManager(
        &utxoManager
    );

    // ========================================================
    // 2. Buat funding block.
    //
    // Dua UTXO independen:
    //
    // funding001: 1000 unit → Alice
    // funding002: 2000 unit → Alice
    //
    // ========================================================

    BerylBlock fundingBlock;

    fundingBlock.header.height = 1;

    BerylTransaction fundingTx1;

    TxOutput fundingOutput1;
    fundingOutput1.address = "berALICE";
    fundingOutput1.amount = 1000;
    fundingOutput1.spent = false;

    fundingTx1.vout.push_back(
        fundingOutput1
    );

    fundingTx1.txid =
        CalculateTxID(
            fundingTx1
        );

    fundingBlock.transactions.push_back(
        fundingTx1
    );

    BerylTransaction fundingTx2;

    TxOutput fundingOutput2;
    fundingOutput2.address = "berALICE";
    fundingOutput2.amount = 2000;
    fundingOutput2.spent = false;

    fundingTx2.vout.push_back(
        fundingOutput2
    );

    fundingTx2.txid =
        CalculateTxID(
            fundingTx2
        );

    fundingBlock.transactions.push_back(
        fundingTx2
    );

    utxoManager.ProcessBlock(
        fundingBlock
    );

    // ========================================================
    // 3. Buat Falcon key.
    // ========================================================

    FalconKey key;

    if (!key.Generate())
    {
        std::cout
            << "KEYGEN FAIL\n";

        return 1;
    }

    std::cout
        << "KEYGEN OK\n";

    // ========================================================
    // 4. TX-A
    //
    // Input  = 1000
    // Output = 999
    // Fee    = 1
    // ========================================================

    BerylTransaction txA;

    TxInput inputA;
    inputA.previousTx =
        fundingTx1.txid;
    inputA.outputIndex = 0;

    txA.vin.push_back(
        inputA
    );

    TxOutput outputA;
    outputA.address = "berBOB";
    outputA.amount = 999;

    txA.vout.push_back(
        outputA
    );

    txA.txid =
        CalculateTxID(
            txA
        );

    if (!SignTransaction(txA, key))
    {
        std::cout
            << "TX-A SIGN FAIL\n";

        return 1;
    }

    std::cout
        << "TX-A SIGN OK\n";

    // ========================================================
    // 5. TX-B
    //
    // Input  = 2000
    // Output = 1999
    // Fee    = 1
    // ========================================================

    BerylTransaction txB;

    TxInput inputB;
    inputB.previousTx =
        fundingTx2.txid;
    inputB.outputIndex = 0;

    txB.vin.push_back(
        inputB
    );

    TxOutput outputB;
    outputB.address = "berCAROL";
    outputB.amount = 1999;

    txB.vout.push_back(
        outputB
    );

    txB.txid =
        CalculateTxID(
            txB
        );

    if (!SignTransaction(txB, key))
    {
        std::cout
            << "TX-B SIGN FAIL\n";

        return 1;
    }

    std::cout
        << "TX-B SIGN OK\n";

    // ========================================================
    // 6. Mempool
    // ========================================================

    Mempool mempool;

    if (!mempool.AddTransaction(
            utxoManager.GetUTXOSet(),
            txA))
    {
        std::cout
            << "TX-A MEMPOOL ADD FAIL\n";

        return 1;
    }

    std::cout
        << "TX-A MEMPOOL OK\n";

    if (!mempool.AddTransaction(
            utxoManager.GetUTXOSet(),
            txB))
    {
        std::cout
            << "TX-B MEMPOOL ADD FAIL\n";

        return 1;
    }

    std::cout
        << "TX-B MEMPOOL OK\n";

    // ========================================================
    // 7. Pastikan dua TX tidak conflict.
    // ========================================================

    if (mempool.Size() != 2)
    {
        std::cout
            << "MEMPOOL SIZE FAIL\n";

        return 1;
    }

    std::cout
        << "MEMPOOL SIZE = 2 OK\n";

    // ========================================================
    // 8. Total fee harus 2 unit.
    // ========================================================

    const uint64_t totalFees =
        mempool.GetTotalFees(
            utxoManager.GetUTXOSet()
        );

    if (totalFees != 2)
    {
        std::cout
            << "TOTAL FEE FAIL\n"
            << "EXPECTED: 2\n"
            << "ACTUAL: "
            << totalFees
            << "\n";

        return 1;
    }

    std::cout
        << "TOTAL FEE = 2 UNIT OK\n";

    // ========================================================
    // 9. Mine block.
    // ========================================================

    BerylBlock block;

    if (!MineBlock(
            block,
            chain,
            mempool,
            "berMINER1"))
    {
        std::cout
            << "MINING FAIL\n";

        return 1;
    }

    std::cout
        << "MINING OK\n";

    // ========================================================
    // 10. Block harus berisi:
    //
    // [0] Coinbase
    // [1] TX-A
    // [2] TX-B
    // ========================================================

    if (block.transactions.size() != 3)
    {
        std::cout
            << "BLOCK TX COUNT FAIL\n"
            << "EXPECTED: 3\n"
            << "ACTUAL: "
            << block.transactions.size()
            << "\n";

        return 1;
    }

    std::cout
        << "BLOCK TX COUNT = 3 OK\n";

    // ========================================================
    // 11. Total fee = 2 unit.
    //
    // Subsidy = 40 BER
    // Coinbase = 40.00000002 BER
    // ========================================================

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(1);

    const uint64_t expectedReward =
        subsidy + 2;

    if (block.reward != expectedReward)
    {
        std::cout
            << "COINBASE REWARD FAIL\n"
            << "EXPECTED: "
            << expectedReward
            << "\n"
            << "ACTUAL: "
            << block.reward
            << "\n";

        return 1;
    }

    std::cout
        << "COINBASE = 40.00000002 BER OK\n";

    // ========================================================
    // 12. Pastikan transaksi benar-benar masuk block.
    // ========================================================

    if (block.transactions[1].txid != txA.txid)
    {
        std::cout
            << "TX-A BLOCK ORDER FAIL\n";

        return 1;
    }

    if (block.transactions[2].txid != txB.txid)
    {
        std::cout
            << "TX-B BLOCK ORDER FAIL\n";

        return 1;
    }

    std::cout
        << "TX-A + TX-B INCLUDED OK\n";

    // ========================================================
    // 13. Mempool harus kosong setelah block accepted.
    // ========================================================

    if (mempool.Size() != 0)
    {
        std::cout
            << "MEMPOOL NOT EMPTY\n"
            << "REMAINING: "
            << mempool.Size()
            << "\n";

        return 1;
    }

    std::cout
        << "MEMPOOL EMPTY AFTER BLOCK OK\n";

    // ========================================================
    // 14. Chain harus memiliki block.
    // ========================================================

    if (chain.GetHeight() != 1)
    {
        std::cout
            << "CHAIN HEIGHT FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: "
            << chain.GetHeight()
            << "\n";

        return 1;
    }

    std::cout
        << "CHAIN HEIGHT = 1 OK\n";

    // ========================================================
    // 15. UTXO hasil TX-A dan TX-B harus tersedia.
    // ========================================================

    if (utxoManager.GetBalance("berBOB") != 999)
    {
        std::cout
            << "BOB BALANCE FAIL\n"
            << "EXPECTED: 999\n"
            << "ACTUAL: "
            << utxoManager.GetBalance("berBOB")
            << "\n";

        return 1;
    }

    if (utxoManager.GetBalance("berCAROL") != 1999)
    {
        std::cout
            << "CAROL BALANCE FAIL\n"
            << "EXPECTED: 1999\n"
            << "ACTUAL: "
            << utxoManager.GetBalance("berCAROL")
            << "\n";

        return 1;
    }

    std::cout
        << "BOB UTXO = 999 OK\n";

    std::cout
        << "CAROL UTXO = 1999 OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL BERYL MULTI TRANSACTION TESTS PASSED\n";

    return 0;
}
