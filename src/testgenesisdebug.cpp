
#include "genesis.h"
#include "blockhash.h"
#include "merkle.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        std::cerr << "STEP 1: START\n";

        std::cerr << "STEP 2: CREATE GENESIS\n";
        BerylBlock g = CreateGenesisBlock();
        std::cerr << "STEP 3: GENESIS CREATED\n";

        std::cerr << "HEIGHT = "
                  << g.header.height << "\n";

        std::cerr << "HASH = "
                  << g.hash << "\n";

        std::cerr << "TX COUNT = "
                  << g.transactions.size() << "\n";

        if (!g.transactions.empty())
        {
            std::cerr << "TXID = "
                      << g.transactions[0].txid << "\n";

            std::cerr << "COINBASE DATA = "
                      << g.transactions[0].coinbaseData
                      << "\n";

            std::cerr << "VIN = "
                      << g.transactions[0].vin.size()
                      << "\n";

            std::cerr << "VOUT = "
                      << g.transactions[0].vout.size()
                      << "\n";

            if (!g.transactions[0].vout.empty())
            {
                std::cerr << "AMOUNT = "
                          << g.transactions[0].vout[0].amount
                          << "\n";

                std::cerr << "ADDRESS = "
                          << g.transactions[0].vout[0].address
                          << "\n";
            }
        }

        std::cerr << "STEP 4: CALCULATE MERKLE\n";

        std::string merkle =
            CalculateMerkleRoot(g.transactions);

        std::cerr << "CALCULATED MERKLE = "
                  << merkle << "\n";

        std::cerr << "STORED MERKLE = "
                  << g.header.merkleRoot << "\n";

        std::cerr << "STEP 5: CALCULATE HASH\n";

        std::string hash =
            GetBlockHash(g.header);

        std::cerr << "CALCULATED HASH = "
                  << hash << "\n";

        std::cerr << "STORED HASH = "
                  << g.hash << "\n";

        std::cerr << "STEP 6: DONE\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: "
                  << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN EXCEPTION\n";
        return 1;
    }

    return 0;
}
