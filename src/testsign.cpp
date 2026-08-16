#include <iostream>
#include <vector>
#include <string>

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


    // Generate key
    if(
        PQCLEAN_FALCON512_CLEAN_crypto_sign_keypair(
            pk.data(),
            sk.data()
        ) != 0
    )
    {
        std::cout << "KEYGEN FAILED\n";
        return 1;
    }

    std::cout << "KEYGEN OK\n";


    std::string message =
        "Beryl transaction test";

    std::vector<unsigned char> signature(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_BYTES
    );

    size_t siglen = 0;


    // Sign
    if(
        PQCLEAN_FALCON512_CLEAN_crypto_sign_signature(
            signature.data(),
            &siglen,
            (unsigned char*)message.data(),
            message.size(),
            sk.data()
        ) != 0
    )
    {
        std::cout << "SIGN FAILED\n";
        return 1;
    }


    std::cout << "SIGN OK\n";
    std::cout << "SIGNATURE SIZE: "
              << siglen
              << "\n";


    // Verify
    if(
        PQCLEAN_FALCON512_CLEAN_crypto_sign_verify(
            signature.data(),
            siglen,
            (unsigned char*)message.data(),
            message.size(),
            pk.data()
        ) != 0
    )
    {
        std::cout << "VERIFY FAILED\n";
        return 1;
    }


    std::cout << "VERIFY OK\n";

    return 0;
}
