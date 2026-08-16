#include "miner.h"
#include "mempool.h"
#include "chain.h"
#include "utxomanager.h"
#include "consensus.h"
#include "transaction.h"

#include <iostream>

int main()
{
    std::cout
        << "=== BERYL MINER MEMPOOL FEE TEST ===\n";

    // ========================================================
    // 1. Chain + UTXO Manager
    // ========================================================

    BerylChain chain;
    UTXOManager utxoManager;

    chain.SetUTXOManager(
        &utxoManager
    );

    // ========================================================
    // 2. Masukkan funding UTXO ke UTXO manager.
    //
    // 1000 unit = 0.00001000 BER
    // ========================================================

    TxOutput funding;
    funding.address = "berALICE";
    funding.amount = 1000;
    funding.spent = false;

    utxoManager.GetUTXOSet();

    // UTXOManager hanya menyediakan const accessor.
    // Untuk test, kita gunakan block funding terlebih dahulu.
    // --------------------------------------------------------

    BerylBlock fundingBlock;

    fundingBlock.header.height = 1;

    BerylTransaction fundingTx;

    TxOutput fundingOutput;
    fundingOutput.address = "berALICE";
    fundingOutput.amount = 1000;

    fundingTx.vout.push_back(
        fundingOutput
    );

    fundingTx.txid =
        CalculateTxID(
            fundingTx
        );

    fundingBlock.transactions.push_back(
        fundingTx
    );

    // ========================================================
    // Catatan:
    // funding block harus diproses melalui UTXO manager.
    // ========================================================

    utxoManager.ProcessBlock(
        fundingBlock
    );

    // ========================================================
    // 3. Buat transaksi Alice -> Bob.
    //
    // Input  = 1000
    // Output = 999
    // Fee    = 1 unit
    // ========================================================

    BerylTransaction tx;

    TxInput input;
    input.previousTx =
        fundingTx.txid;

    input.outputIndex = 0;

    tx.vin.push_back(
        input
    );

    TxOutput bob;
    bob.address = "berBOB";
    bob.amount = 999;

    tx.vout.push_back(
        bob
    );

    // ========================================================
    // 4. Falcon signing
    // ========================================================

    FalconKey key;

    if (!key.Generate())
    {
        std::cout
            << "KEYGEN FAIL\n";

        return 1;
    }

    tx.txid =
        CalculateTxID(
            tx
        );

    if (!SignTransaction(tx, key))
    {
        std::cout
            << "SIGN FAIL\n";

        return 1;
    }

    std::cout
        << "SIGN OK\n";

    // ========================================================
    // 5. Masukkan ke mempool
    // ========================================================

    Mempool mempool;

    if (!mempool.AddTransaction(
            utxoManager.GetUTXOSet(),
            tx))
    {
        std::cout
            << "MEMPOOL ADD FAIL\n";

        return 1;
    }

    std::cout
        << "MEMPOOL ADD OK\n";

    // ========================================================
    // 6. Pastikan fee = 1 unit
    // ========================================================

    const uint64_t fee =
        mempool.GetTotalFees(
            utxoManager.GetUTXOSet()
        );

    if (fee != 1)
    {
        std::cout
            << "FEE FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: "
            << fee
            << "\n";

        return 1;
    }

    std::cout
        << "FEE = 1 UNIT OK\n";

    // ========================================================
    // 7. Mine block menggunakan mempool.
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
    // 8. Subsidy
    // ========================================================

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            1
        );

    if (subsidy !=
        40ULL * BerylConsensus::COIN)
    {
        std::cout
            << "SUBSIDY FAIL\n";

        return 1;
    }

    std::cout
        << "SUBSIDY = 40 BER OK\n";

    // ========================================================
    // 9. Coinbase harus subsidy + fee.
    // ========================================================

    const uint64_t expectedCoinbase =
        subsidy + 1;

    if (block.reward != expectedCoinbase)
    {
        std::cout
            << "COINBASE REWARD FAIL\n"
            << "EXPECTED: "
            << expectedCoinbase
            << "\n"
            << "ACTUAL: "
            << block.reward
            << "\n";

        return 1;
    }

    if (block.transactions.empty())
    {
        std::cout
            << "BLOCK TRANSACTIONS FAIL\n";

        return 1;
    }

    if (block.transactions[0].vout.empty())
    {
        std::cout
            << "COINBASE OUTPUT FAIL\n";

        return 1;
    }

    if (block.transactions[0].vout[0].amount !=
        expectedCoinbase)
    {
        std::cout
            << "COINBASE OUTPUT AMOUNT FAIL\n";

        return 1;
    }

    std::cout
        << "COINBASE = 40.00000001 BER OK\n";

    // ========================================================
    // 10. Harus ada 2 transaksi:
    //
    // [0] coinbase
    // [1] Alice -> Bob
    // ========================================================

    if (block.transactions.size() != 2)
    {
        std::cout
            << "BLOCK TX COUNT FAIL\n"
            << "EXPECTED: 2\n"
            << "ACTUAL: "
            << block.transactions.size()
            << "\n";

        return 1;
    }

    std::cout
        << "MEMPOOL TX INCLUDED OK\n";

    // ========================================================
    // FINAL
    // ========================================================

    std::cout
        << "\nALL BERYL MINER MEMPOOL FEE TESTS PASSED\n";

    return 0;
}
