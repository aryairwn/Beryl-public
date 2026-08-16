#include "wallet.h"
#include <iostream>


int main()
{

    BerylWallet wallet;


    if(wallet.Generate())
    {

        std::cout
        << "ADDRESS: "
        << wallet.GetAddress()
        << "\n";


        if(wallet.Save("../data/wallet.dat"))
        {
            std::cout
            << "WALLET SAVED\n";
        }
        else
        {
            std::cout
            << "SAVE FAILED\n";
        }

    }


    return 0;
}
