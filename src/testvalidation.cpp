#include "validation.h"
#include "genesis.h"

#include <iostream>


int main()
{

    BerylBlock block =
        CreateGenesisBlock();


    if(CheckBlock(block,"0"))
    {
        std::cout
        << "BLOCK VALID\n";
    }
    else
    {
        std::cout
        << "BLOCK INVALID\n";
    }


    return 0;
}
