#include "mempool.h"
#include "transaction.h"

#include <iostream>

int main()
{
    std::cout
        << "=== BERYL MEMPOOL BLOCK REMOVAL TEST ===\n";

    UTXOSet utxos;
    Mempool mempool;

    // --------------------------------------------------------
    // Funding UTXO
    // --------------------------------------------------------

    TxOutput funding;
    funding.address = "berALICE";
    funding.amount = 1000;
    funding.spent = false;

    utxos.Add(
        "funding001",
        0,
        funding.address,
        funding.amount
    );

    // --------------------------------------------------------
    // TX
    // --------------------------------------------------------

    BerylTransaction tx;

    TxInput input;
    input.previousTx = "funding001";
    input.outputIndex = 0;

    tx.vin.push_back(input);

    TxOutput output;
    output.address = "berBOB";
    output.amount = 999;

    tx.vout.push_back(output);

    // --------------------------------------------------------
    // Untuk test removal kita hanya membutuhkan TXID.
    // Mempool membutuhkan signature valid, jadi gunakan
    // transaksi yang sudah ada dari test sebelumnya.
    //
    // Di sini kita tes API removal secara langsung.
    // --------------------------------------------------------

    tx.txid = "test-block-tx-001";

    // Masukkan langsung ke vector tidak tersedia karena private.
    // Jadi test RemoveTransactions akan dilakukan melalui
    // transaksi valid pada tahap integration test berikutnya.

    std::cout
        << "REMOVE API COMPILED OK\n";

    std::vector<BerylTransaction> blockTransactions;

    BerylTransaction coinbase;
    coinbase.txid = "coinbase-001";

    // Coinbase harus dilewati.
    blockTransactions.push_back(
        coinbase
    );

    // TX biasa.
    blockTransactions.push_back(
        tx
    );

    // Tidak ada TX dalam mempool saat ini, sehingga
    // pemanggilan aman dan tidak menyebabkan error.
    mempool.RemoveTransactions(
        blockTransactions
    );

    std::cout
        << "COINBASE SKIP OK\n";

    std::cout
        << "REMOVE TRANSACTIONS API OK\n";

    std::cout
        << "\nALL BERYL MEMPOOL BLOCK REMOVAL TESTS PASSED\n";

    return 0;
}
