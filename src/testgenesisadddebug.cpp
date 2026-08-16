
#include "genesis.h"
#include "chain.h"
#include "utxomanager.h"
#include "blockvalidation.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        std::cerr << "STEP 1: START\n";

        BerylBlock genesis = CreateGenesisBlock();

        std::cerr << "STEP 2: GENESIS CREATED\n";
        std::cerr << "HEIGHT = "
                  << genesis.header.height << "\n";

        std::cerr << "STEP 3: CREATE CHAIN\n";

        BerylChain chain;

        std::cerr << "STEP 4: CHAIN CREATED\n";

        UTXOManager utxo;

        std::cerr << "STEP 5: UTXO CREATED\n";

        chain.SetUTXOManager(&utxo);

        std::cerr << "STEP 6: UTXO MANAGER CONNECTED\n";

        std::cerr << "CHAIN HEIGHT = "
                  << chain.GetHeight() << "\n";

        std::cerr << "LAST HASH = "
                  << chain.GetLastHash() << "\n";

        std::cerr << "STEP 7: DIRECT VALIDATE BLOCK\n";

        bool valid =
            ValidateBlock(
                genesis,
                chain,
                utxo
            );

        std::cerr << "VALIDATE RESULT = "
                  << (valid ? "TRUE" : "FALSE")
                  << "\n";

        std::cerr << "STEP 8: ADD BLOCK\n";

        bool added = chain.AddBlock(genesis);

        std::cerr << "ADD RESULT = "
                  << (added ? "TRUE" : "FALSE")
                  << "\n";

        std::cerr << "FINAL HEIGHT = "
                  << chain.GetHeight()
                  << "\n";

        return added ? 0 : 1;
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
}
