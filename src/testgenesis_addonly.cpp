#include "genesis.h"
#include "chain.h"
#include "utxomanager.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        std::cout << "STEP 1: START\n";

        UTXOManager utxo;
        BerylChain chain;

        std::cout << "STEP 2: OBJECTS CREATED\n";

        chain.SetUTXOManager(&utxo);

        std::cout << "STEP 3: UTXO MANAGER CONNECTED\n";

        BerylBlock genesis = CreateGenesisBlock();

        std::cout << "STEP 4: GENESIS CREATED\n";
        std::cout << "HEIGHT = "
                  << genesis.header.height << "\n";
        std::cout << "HASH = "
                  << genesis.hash << "\n";
        std::cout << "TX COUNT = "
                  << genesis.transactions.size() << "\n";

        std::cout << "STEP 5: CALL AddBlock()\n";

        bool result = chain.AddBlock(genesis);

        std::cout << "STEP 6: AddBlock RETURNED\n";
        std::cout << "RESULT = "
                  << (result ? "TRUE" : "FALSE")
                  << "\n";

        if (!result)
        {
            std::cout << "GENESIS ADD FAILED\n";
            return 1;
        }

        std::cout << "CHAIN HEIGHT = "
                  << chain.GetHeight() << "\n";

        std::cout << "GENESIS BALANCE = "
                  << utxo.GetBalance("berGENESIS")
                  << "\n";

        std::cout << "ALL GENESIS ADD TESTS PASSED\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: "
                  << e.what() << "\n";
        return 1;
    }
}
