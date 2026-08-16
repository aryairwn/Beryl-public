#include "mempool.h"
#include "crypto/falcon/falcon.h"

#include <iostream>

int main()
{
    std::cout
        << "=== BERYL MEMPOOL TEST ===\n";

    UTXOSet utxos;

    // --------------------------------------------------------
    // Funding UTXO = 1000 unit
    // --------------------------------------------------------

    TxOutput source;
    source.address = "berALICE";
    source.amount = 1000;
    source.spent = false;

    utxos.Add(
        "funding001",
        0,
        source.address,
        source.amount
    );

    // --------------------------------------------------------
    // Transaksi:
    //
    // Input  = 1000
    // Output = 999
    // Fee    = 1 unit
    // --------------------------------------------------------

    BerylTransaction tx;

    TxInput input;
    input.previousTx = "funding001";
    input.outputIndex = 0;

    tx.vin.push_back(input);

    TxOutput output;
    output.address = "berBOB";
    output.amount = 999;
    output.spent = false;

    tx.vout.push_back(output);

    tx.txid = CalculateTxID(tx);

    // --------------------------------------------------------
    // Sign transaksi dengan Falcon.
    // --------------------------------------------------------

    FalconKey key;

    if (!key.Generate())
    {
        std::cout
            << "KEYGEN FAIL\\n";

        return 1;
    }

    if (!SignTransaction(tx, key))
    {
        std::cout
            << "SIGN FAIL\\n";

        return 1;
    }

    std::cout
        << "SIGN OK\\n";

    // --------------------------------------------------------
    // Tambahkan ke mempool.
    // --------------------------------------------------------

    Mempool mempool;

    if (!mempool.AddTransaction(utxos, tx))
    {
        std::cout
            << "ADD TRANSACTION FAIL\n";

        return 1;
    }

    std::cout
        << "TRANSACTION ADDED OK\n";

    // --------------------------------------------------------
    // Size
    // --------------------------------------------------------

    if (mempool.Size() != 1)
    {
        std::cout
            << "MEMPOOL SIZE FAIL\n";

        return 1;
    }

    std::cout
        << "MEMPOOL SIZE OK\n";

    // --------------------------------------------------------
    // Fee
    // --------------------------------------------------------

    uint64_t fee =
        mempool.GetTotalFees(utxos);

    if (fee != 1)
    {
        std::cout
            << "MEMPOOL FEE FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: "
            << fee
            << "\n";

        return 1;
    }

    std::cout
        << "MEMPOOL FEE = 1 UNIT OK\n";

    // --------------------------------------------------------
    // Duplicate
    // --------------------------------------------------------

    if (mempool.AddTransaction(utxos, tx))
    {
        std::cout
            << "DUPLICATE ACCEPTED FAIL\n";

        return 1;
    }

    std::cout
        << "DUPLICATE REJECTED OK\n";

    // --------------------------------------------------------
    // Contains
    // --------------------------------------------------------

    if (!mempool.Contains(tx.txid))
    {
        std::cout
            << "CONTAINS FAIL\n";

        return 1;
    }

    std::cout
        << "CONTAINS OK\n";

    // --------------------------------------------------------
    // Remove
    // --------------------------------------------------------

    if (!mempool.RemoveTransaction(tx.txid))
    {
        std::cout
            << "REMOVE FAIL\n";

        return 1;
    }

    if (mempool.Size() != 0)
    {
        std::cout
            << "EMPTY MEMPOOL FAIL\n";

        return 1;
    }

    std::cout
        << "REMOVE OK\n";

    std::cout
        << "\nALL BERYL MEMPOOL TESTS PASSED\n";

    return 0;
}
