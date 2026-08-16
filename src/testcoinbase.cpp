#include "coinbase.h"
#include "consensus.h"
#include "crypto/falcon/falcon.h"

#include <iostream>
#include <vector>

int main()
{
    std::cout << "=== BERYL COINBASE REWARD TEST ===\n";

    FalconKey key;

    if (!key.Generate())
    {
        std::cout << "KEYGEN FAIL\n";
        return 1;
    }

    // ========================================================
    // UTXO 100 unit
    // ========================================================

    UTXOSet utxos;

    TxOutput inputOut;
    inputOut.address = "berSENDER";
    inputOut.amount = 100;
    inputOut.spent = false;

    const std::string prevTx =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    utxos.Add(
        prevTx,
        0,
        inputOut
    );

    // ========================================================
    // TX: input 100 - output 99 = fee 1 unit
    // ========================================================

    BerylTransaction tx;

    TxInput input;
    input.previousTx = prevTx;
    input.outputIndex = 0;

    tx.vin.push_back(input);

    TxOutput output;
    output.address = "berRECEIVER";
    output.amount = 99;
    output.spent = false;

    tx.vout.push_back(output);

    if (!SignTransaction(tx, key))
    {
        std::cout << "SIGN FAIL\n";
        return 1;
    }

    uint64_t fee =
        CalculateTransactionFee(utxos, tx);

    if (fee != 1)
    {
        std::cout
            << "TRANSACTION FEE FAIL\n"
            << "EXPECTED: 1\n"
            << "ACTUAL: " << fee << "\n";
        return 1;
    }

    std::cout << "TRANSACTION FEE = 1 UNIT OK\n";

    // ========================================================
    // Block fees
    // ========================================================

    std::vector<BerylTransaction> transactions = {
        tx
    };

    uint64_t blockFees =
        CalculateBlockFees(
            utxos,
            transactions
        );

    if (blockFees != 1)
    {
        std::cout << "BLOCK FEE FAIL\n";
        return 1;
    }

    std::cout << "BLOCK FEES = 1 UNIT OK\n";

    // ========================================================
    // Coinbase reward
    // ========================================================

    const int height = 0;

    uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(height);

    uint64_t reward =
        CalculateCoinbaseReward(
            height,
            utxos,
            transactions
        );

    uint64_t expected =
        subsidy + 1;

    if (reward != expected)
    {
        std::cout
            << "COINBASE REWARD FAIL\n"
            << "EXPECTED: " << expected << "\n"
            << "ACTUAL: " << reward << "\n";
        return 1;
    }

    std::cout << "SUBSIDY + FEE OK\n";

    if (subsidy != 40ULL * BerylConsensus::COIN)
    {
        std::cout << "YEAR 0 SUBSIDY FAIL\n";
        return 1;
    }

    std::cout
        << "YEAR 0 SUBSIDY = 40 BER OK\n";

    // ========================================================
    // Create coinbase
    // ========================================================

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            "berMINER",
            reward
        );

    if (!coinbase.vin.empty())
    {
        std::cout << "COINBASE INPUT FAIL\n";
        return 1;
    }

    if (coinbase.vout.size() != 1)
    {
        std::cout << "COINBASE OUTPUT COUNT FAIL\n";
        return 1;
    }

    if (coinbase.vout[0].amount != reward)
    {
        std::cout << "COINBASE AMOUNT FAIL\n";
        return 1;
    }

    std::cout << "COINBASE AMOUNT OK\n";

    // ========================================================
    // No-fee block
    // ========================================================

    std::vector<BerylTransaction> noFeeTransactions;

    uint64_t noFeeReward =
        CalculateCoinbaseReward(
            height,
            utxos,
            noFeeTransactions
        );

    if (noFeeReward != subsidy)
    {
        std::cout
            << "NO-FEE COINBASE FAIL\n";
        return 1;
    }

    std::cout
        << "NO-FEE COINBASE = SUBSIDY OK\n";

    std::cout
        << "\nALL BERYL COINBASE REWARD TESTS PASSED\n";

    return 0;
}
