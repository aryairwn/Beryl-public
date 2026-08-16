#include "genesis.h"

#include <iostream>


int main()
{

    BerylBlock g =
    CreateGenesisBlock();


    std::cout
    << "BERYL GENESIS\n";


    std::cout
    << "HASH: "
    << g.hash
    << "\n";


    std::cout
    << "REWARD: "
    << g.reward
    << " BER\n";


    return 0;

}
