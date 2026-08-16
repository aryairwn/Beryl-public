#include <iostream>
#include <vector>

extern "C" {
#include "crypto/falcon/PQClean/crypto_sign/falcon-512/clean/api.h"
}

int main()
{
    std::vector<unsigned char> pk(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_PUBLICKEYBYTES
    );

    std::vector<unsigned char> sk(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_SECRETKEYBYTES
    );


    int r =
    PQCLEAN_FALCON512_CLEAN_crypto_sign_keypair(
        pk.data(),
        sk.data()
    );


    if(r == 0)
    {
        std::cout << "FALCON KEYGEN OK\n";
        std::cout << "PUBLIC KEY: "
                  << pk.size()
                  << "\n";

        std::cout << "PRIVATE KEY: "
                  << sk.size()
                  << "\n";
    }
    else
    {
        std::cout << "FAILED\n";
    }

    return 0;
}
