#include "miner.h"
#include "chain.h"
#include "utxomanager.h"
#include "consensus.h"

#include <iostream>

int main()
{
    std::cout
        << "=== BERYL MINER TEST ===\n";

    // --------------------------------------------------------
    // 1. Chain + UTXO Manager
    // --------------------------------------------------------

    BerylChain chain;
    UTXOManager utxoManager;

    chain.SetUTXOManager(
        &utxoManager
    );

    // --------------------------------------------------------
    // 2. Mine block pertama
    // --------------------------------------------------------

    BerylBlock block;

    if (!MineBlock(
            block,
            chain,
            "berMINER1"))
    {
        std::cout
            << "MINING FAIL\n";

        return 1;
    }

    std::cout
        << "MINING OK\n";

    // --------------------------------------------------------
    // 3. Height
    // --------------------------------------------------------

    if (block.header.height != 1)
    {
        std::cout
            << "HEIGHT FAIL\n";

        return 1;
    }

    std::cout
        << "HEIGHT 1 OK\n";

    // --------------------------------------------------------
    // 4. Reward Year 0 = 40 BER
    // --------------------------------------------------------

    const uint64_t expectedReward =
        BerylConsensus::GetBlockSubsidy(1);

    if (expectedReward != 40ULL * BerylConsensus::COIN)
    {
        std::cout
            << "CONSENSUS REWARD BASE FAIL\n";

        return 1;
    }

    if (block.reward != expectedReward)
    {
        std::cout
            << "BLOCK REWARD FAIL\n"
            << "EXPECTED: "
            << expectedReward
            << "\n"
            << "ACTUAL: "
            << block.reward
            << "\n";

        return 1;
    }

    std::cout
        << "REWARD 40 BER OK\n";

    // --------------------------------------------------------
    // 5. Coinbase
    // --------------------------------------------------------

    if (block.transactions.size() != 1)
    {
        std::cout
            << "TRANSACTION COUNT FAIL\n";

        return 1;
    }

    const BerylTransaction& coinbase =
        block.transactions[0];

    if (!coinbase.vin.empty())
    {
        std::cout
            << "COINBASE INPUT FAIL\n";

        return 1;
    }

    if (coinbase.vout.size() != 1)
    {
        std::cout
            << "COINBASE OUTPUT FAIL\n";

        return 1;
    }

    if (coinbase.vout[0].amount != expectedReward)
    {
        std::cout
            << "COINBASE AMOUNT FAIL\n";

        return 1;
    }

    std::cout
        << "COINBASE REWARD OK\n";

    // --------------------------------------------------------
    // 6. Merkle Root
    // --------------------------------------------------------

    if (block.header.merkleRoot.empty())
    {
        std::cout
            << "MERKLE ROOT FAIL\n";

        return 1;
    }

    std::cout
        << "MERKLE ROOT OK\n";

    // --------------------------------------------------------
    // 7. PoW
    // --------------------------------------------------------

    if (block.hash.empty())
    {
        std::cout
            << "BLOCK HASH FAIL\n";

        return 1;
    }

    if (block.hash[0] != '0')
    {
        std::cout
            << "POW FAIL\n";

        return 1;
    }

    std::cout
        << "POW OK\n";

    // --------------------------------------------------------
    // 8. Add block melalui chain validation
    // --------------------------------------------------------

    if (!chain.AddBlock(block))
    {
        std::cout
            << "ADD BLOCK FAIL\n";

        return 1;
    }

    std::cout
        << "ADD BLOCK OK\n";

    // --------------------------------------------------------
    // 9. Coinbase masuk UTXO
    // --------------------------------------------------------

    const uint64_t balance =
        utxoManager.GetBalance(
            "berMINER1"
        );

    if (balance != expectedReward)
    {
        std::cout
            << "MINER UTXO FAIL\n"
            << "EXPECTED: "
            << expectedReward
            << "\n"
            << "ACTUAL: "
            << balance
            << "\n";

        return 1;
    }

    std::cout
        << "MINER UTXO OK\n";

    // --------------------------------------------------------
    // FINAL
    // --------------------------------------------------------

    std::cout
        << "\nALL BERYL MINER TESTS PASSED\n";

    return 0;
}
