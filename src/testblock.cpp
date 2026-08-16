#include "block.h"
#include "blockhash.h"

#include <iostream>
#include <ctime>


int main()
{

    std::cout
    << "BERYL BLOCK TEST\n";


    BerylBlock block;

    block.header.previousHash =
    "0";


    block.header.timestamp =
    time(nullptr);


    block.header.nonce = 0;


    block.reward = 40;


    block.hash =
    GetBlockHash(block.header);



    std::cout
    << "HASH:\n"
    << block.hash
    << "\n";


    std::cout
    << "REWARD: "
    << block.reward
    << " BER\n";


    return 0;
}
