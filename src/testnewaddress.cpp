#include "wallet.h"

#include <iostream>

int main()
{
    BerylWallet wallet;

    std::cout
        << "=== BERYL NEW ADDRESS TEST ===\n";

    if(!wallet.Load("../data/wallet.dat"))
    {
        std::cerr
            << "WALLET LOAD FAILED\n";
        return 1;
    }

    std::string oldAddress =
        wallet.GetAddress();

    std::cout
        << "OLD ADDRESS: "
        << oldAddress
        << "\n";

    std::string newAddress;

    if(!wallet.GenerateNewAddress(newAddress))
    {
        std::cerr
            << "NEW ADDRESS GENERATION FAILED\n";
        return 1;
    }

    std::cout
        << "NEW ADDRESS: "
        << newAddress
        << "\n";

    std::cout
        << "OLD ADDRESS AFTER: "
        << wallet.GetAddress()
        << "\n";

    if(oldAddress != wallet.GetAddress())
    {
        std::cerr
            << "ERROR: OLD ADDRESS CHANGED\n";
        return 1;
    }

    if(newAddress.empty())
    {
        std::cerr
            << "ERROR: NEW ADDRESS EMPTY\n";
        return 1;
    }

    if(newAddress == oldAddress)
    {
        std::cerr
            << "ERROR: NEW ADDRESS SAME AS OLD\n";
        return 1;
    }

    std::cout
        << "TEST PASSED\n";

    return 0;
}
