
#include "genesis.h"
#include "chain.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        std::cerr << "START\n";

        BerylBlock g = CreateGenesisBlock();

        std::cerr << "GENESIS OK\n";
        std::cerr << "HASH = " << g.hash << "\n";

        BerylChain chain;

        std::cerr << "CHAIN OK\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }
}
