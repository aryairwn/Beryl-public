
#include "genesis.h"
#include "chain.h"
#include "utxomanager.h"
#include "blockhash.h"
#include "merkle.h"
#include "consensus.h"
#include "transaction.h"

#include <iostream>

int main()
{
    std::cout << "=== BERYL GENESIS VALIDATION DEBUG ===\n";

    BerylChain chain;
    UTXOManager utxo;

    chain.SetUTXOManager(&utxo);

    BerylBlock g = CreateGenesisBlock();

    std::cout << "HEIGHT: "
              << g.header.height << "\n";

    std::cout << "PREVIOUS: "
              << g.header.previousHash << "\n";

    std::cout << "CHAIN LAST: "
              << chain.GetLastHash() << "\n";

    std::cout << "DIFFICULTY: "
              << g.header.difficulty << "\n";

    std::cout << "HASH: "
              << g.hash << "\n";

    std::cout << "MERKLE: "
              << g.header.merkleRoot << "\n";

    std::cout << "TX COUNT: "
              << g.transactions.size() << "\n";

    if (g.transactions.empty())
    {
        std::cout << "FAIL: NO TRANSACTION\n";
        return 1;
    }

    const BerylTransaction& cb =
        g.transactions.front();

    std::cout << "COINBASE TXID: "
              << cb.txid << "\n";

    std::cout << "COINBASE DATA: "
              << cb.coinbaseData << "\n";

    std::cout << "COINBASE VIN: "
              << cb.vin.size() << "\n";

    std::cout << "COINBASE VOUT: "
              << cb.vout.size() << "\n";

    if (!cb.vout.empty())
    {
        std::cout << "COINBASE AMOUNT: "
                  << cb.vout[0].amount << "\n";

        std::cout << "COINBASE ADDRESS: "
                  << cb.vout[0].address << "\n";
    }

    // Height
    if (g.header.height != 1)
        std::cout << "FAIL: HEIGHT\n";
    else
        std::cout << "HEIGHT OK\n";

    // Previous hash
    if (g.header.previousHash != chain.GetLastHash())
        std::cout << "FAIL: PREVIOUS HASH\n";
    else
        std::cout << "PREVIOUS HASH OK\n";

    // Merkle
    std::string merkle =
        CalculateMerkleRoot(g.transactions);

    std::cout << "CALCULATED MERKLE: "
              << merkle << "\n";

    if (merkle != g.header.merkleRoot)
        std::cout << "FAIL: MERKLE\n";
    else
        std::cout << "MERKLE OK\n";

    // Hash
    std::string calculatedHash =
        GetBlockHash(g.header);

    std::cout << "CALCULATED HASH: "
              << calculatedHash << "\n";

    if (calculatedHash != g.hash)
        std::cout << "FAIL: BLOCK HASH\n";
    else
        std::cout << "BLOCK HASH OK\n";

    // PoW
    if (g.hash.empty() || g.hash[0] != '0')
        std::cout << "FAIL: POW\n";
    else
        std::cout << "POW OK\n";

    // Subsidy
    uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            g.header.height
        );

    std::cout << "SUBSIDY: "
              << subsidy << "\n";

    if (!cb.vout.empty())
    {
        if (cb.vout[0].amount > subsidy)
            std::cout << "FAIL: COINBASE > SUBSIDY\n";
        else
            std::cout << "COINBASE REWARD OK\n";
    }

    // Finally
    std::cout << "\nTRY ADD BLOCK...\n";

    if (chain.AddBlock(g))
        std::cout << "GENESIS ADD OK\n";
    else
        std::cout << "GENESIS ADD FAIL\n";

    return 0;
}
