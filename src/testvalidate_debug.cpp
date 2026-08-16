#include "genesis.h"
#include "blockvalidation.h"
#include "chain.h"
#include "utxomanager.h"
#include "blockhash.h"
#include "merkle.h"
#include "consensus.h"
#include "transaction.h"

#include <iostream>
#include <cstdint>

int main()
{
    std::cout << "=== BERYL VALIDATE DEBUG ===\n";

    UTXOManager utxo;
    BerylChain chain;

    chain.SetUTXOManager(&utxo);

    BerylBlock g = CreateGenesisBlock();

    std::cout << "1. TX COUNT: "
              << g.transactions.size() << "\n";

    if (g.transactions.empty())
    {
        std::cout << "FAIL: transactions.empty()\n";
        return 1;
    }

    std::cout << "2. HEIGHT: "
              << g.header.height
              << " EXPECTED: "
              << chain.GetHeight() + 1
              << "\n";

    std::cout << "3. PREVIOUS HASH: "
              << g.header.previousHash
              << "\n";

    std::cout << "   CHAIN LAST HASH: "
              << chain.GetLastHash()
              << "\n";

    std::string merkle =
        CalculateMerkleRoot(g.transactions);

    std::cout << "4. MERKLE CALCULATED: "
              << merkle << "\n";

    std::cout << "   MERKLE STORED:     "
              << g.header.merkleRoot << "\n";

    std::string hash =
        GetBlockHash(g.header);

    std::cout << "5. HASH CALCULATED: "
              << hash << "\n";

    std::cout << "   HASH STORED:     "
              << g.hash << "\n";

    std::cout << "6. DIFFICULTY: "
              << g.header.difficulty << "\n";

    std::cout << "7. HASH FIRST CHAR: "
              << (g.hash.empty() ? '?' : g.hash[0])
              << "\n";

    const BerylTransaction& coinbase =
        g.transactions.front();

    std::cout << "8. COINBASE VIN: "
              << coinbase.vin.size() << "\n";

    std::cout << "9. COINBASE VOUT: "
              << coinbase.vout.size() << "\n";

    uint64_t coinbaseAmount = 0;

    for (const auto& out : coinbase.vout)
    {
        std::cout << "   ADDRESS: "
                  << out.address
                  << "\n";

        std::cout << "   AMOUNT: "
                  << out.amount
                  << "\n";

        coinbaseAmount += out.amount;
    }

    std::cout << "10. COINBASE TOTAL: "
              << coinbaseAmount << "\n";

    uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            g.header.height
        );

    std::cout << "11. SUBSIDY: "
              << subsidy << "\n";

    std::cout << "12. BLOCK REWARD FIELD: "
              << g.reward << "\n";

    std::cout << "13. MAX COINBASE: "
              << subsidy << "\n";

    std::cout << "\n=== DIRECT ValidateBlock() ===\n";

    bool valid =
        ValidateBlock(
            g,
            chain,
            utxo
        );

    std::cout << "VALIDATE RESULT: "
              << (valid ? "TRUE" : "FALSE")
              << "\n";

    return valid ? 0 : 1;
}
