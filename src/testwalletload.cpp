#include "wallet.h"
#include <iostream>

int main()
{
    std::cout << "=== BERYL WALLET RESTORE TEST ===\n";

    BerylWallet wallet;

    if(!wallet.Load("../data/wallet.dat"))
    {
        std::cout << "WALLET LOAD FAILED\n";
        return 1;
    }

    std::cout << "WALLET LOAD OK\n";

    std::cout
        << "ADDRESS: "
        << wallet.GetAddress()
        << "\n";

    std::cout
        << "PUBLIC KEY HEX LENGTH: "
        << wallet.GetPublicKeyHex().size()
        << "\n";

    std::cout
        << "PRIVATE KEY RESTORED: "
        << (wallet.GetPrivateKeyHex().empty()
                ? "NO"
                : "YES")
        << "\n";

    return 0;
}
