#include "transaction.h"

#include <iostream>
#include <vector>

int main()
{
    std::cout << "=== BERYL BLOCK FEE TEST ===\n";

    UTXOSet utxos;

    // ========================================================
    // UTXO 1 = 100 unit
    // TX1 output = 99 unit
    // Fee = 1 unit
    // ========================================================

    TxOutput u1;
    u1.address = "berA";
    u1.amount = 100;
    u1.spent = false;

    const std::string txid1 =
        "1111111111111111111111111111111111111111";

    utxos.Add(
        txid1,
        0,
        u1
    );

    BerylTransaction tx1;

    TxInput in1;
    in1.previousTx = txid1;
    in1.outputIndex = 0;

    tx1.vin.push_back(in1);

    TxOutput out1;
    out1.address = "berB";
    out1.amount = 99;
    out1.spent = false;

    tx1.vout.push_back(out1);

    // TX harus ditandatangani agar ValidateTransaction()
    // dan CalculateTransactionFee() menerima transaksi.
    FalconKey key;

    if (!key.Generate())
    {
        std::cout << "KEYGEN FAIL\n";
        return 1;
    }

    if (!SignTransaction(tx1, key))
    {
        std::cout << "TX1 SIGN FAIL\n";
        return 1;
    }

    // ========================================================
    // UTXO 2 = 200 unit
    // TX2 output = 199 unit
    // Fee = 1 unit
    // ========================================================

    TxOutput u2;
    u2.address = "berC";
    u2.amount = 200;
    u2.spent = false;

    const std::string txid2 =
        "2222222222222222222222222222222222222222";

    utxos.Add(
        txid2,
        0,
        u2
    );

    BerylTransaction tx2;

    TxInput in2;
    in2.previousTx = txid2;
    in2.outputIndex = 0;

    tx2.vin.push_back(in2);

    TxOutput out2;
    out2.address = "berD";
    out2.amount = 199;
    out2.spent = false;

    tx2.vout.push_back(out2);

    if (!SignTransaction(tx2, key))
    {
        std::cout << "TX2 SIGN FAIL\n";
        return 1;
    }

    // ========================================================
    // Test fee TX1
    // ========================================================

    uint64_t fee1 =
        CalculateTransactionFee(
            utxos,
            tx1
        );

    if (fee1 != 1)
    {
        std::cout
            << "TX1 FEE FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: "
            << fee1
            << "\n";

        return 1;
    }

    std::cout
        << "TX1 FEE = 1 UNIT OK\n";

    // ========================================================
    // Test fee TX2
    // ========================================================

    uint64_t fee2 =
        CalculateTransactionFee(
            utxos,
            tx2
        );

    if (fee2 != 1)
    {
        std::cout
            << "TX2 FEE FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: "
            << fee2
            << "\n";

        return 1;
    }

    std::cout
        << "TX2 FEE = 1 UNIT OK\n";

    // ========================================================
    // Block fee
    // ========================================================

    std::vector<BerylTransaction> blockTxs = {
        tx1,
        tx2
    };

    uint64_t totalFees =
        CalculateBlockFees(
            utxos,
            blockTxs
        );

    if (totalFees != 2)
    {
        std::cout
            << "TOTAL BLOCK FEE FAIL\n"
            << "EXPECTED: 2\n"
            << "ACTUAL: "
            << totalFees
            << "\n";

        return 1;
    }

    std::cout
        << "TOTAL BLOCK FEE = 2 UNIT OK\n";

    // ========================================================
    // Coinbase harus tidak dihitung sebagai fee
    // ========================================================

    BerylTransaction coinbase;

    TxOutput coinbaseOut;
    coinbaseOut.address = "berMINER";
    coinbaseOut.amount = 4000000000ULL;
    coinbaseOut.spent = false;

    coinbase.vout.push_back(
        coinbaseOut
    );

    std::vector<BerylTransaction> withCoinbase = {
        coinbase,
        tx1,
        tx2
    };

    uint64_t totalWithCoinbase =
        CalculateBlockFees(
            utxos,
            withCoinbase
        );

    if (totalWithCoinbase != 2)
    {
        std::cout
            << "COINBASE EXCLUSION FAIL\n";
        return 1;
    }

    std::cout
        << "COINBASE EXCLUDED OK\n";

    // ========================================================
    // Coinbase saja = fee 0
    // ========================================================

    std::vector<BerylTransaction> onlyCoinbase = {
        coinbase
    };

    if (CalculateBlockFees(
            utxos,
            onlyCoinbase) != 0)
    {
        std::cout
            << "COINBASE ONLY FEE FAIL\n";
        return 1;
    }

    std::cout
        << "COINBASE ONLY FEE = 0 OK\n";

    std::cout
        << "\nALL BERYL BLOCK FEE TESTS PASSED\n";

    return 0;
}
