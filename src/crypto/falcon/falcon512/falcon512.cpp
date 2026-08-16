#include "falcon512.h"
#include <vector>

extern "C" {
#include "../PQClean/crypto_sign/falcon-512/clean/api.h"
}

bool Falcon512_KeyGen(
    std::vector<unsigned char>& publicKey,
    std::vector<unsigned char>& privateKey
)
{

    publicKey.resize(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_PUBLICKEYBYTES
    );


    privateKey.resize(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_SECRETKEYBYTES
    );


    int result =
    PQCLEAN_FALCON512_CLEAN_crypto_sign_keypair(
        publicKey.data(),
        privateKey.data()
    );


    return result == 0;
}



bool Falcon512_KeyGenFromSeed(
    const std::vector<unsigned char>& seed,
    std::vector<unsigned char>& publicKey,
    std::vector<unsigned char>& privateKey
)
{
    /*
     * Falcon-512 seeded API membutuhkan tepat 48 byte.
     */
    if (seed.size() != 48)
        return false;

    publicKey.resize(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_PUBLICKEYBYTES
    );

    privateKey.resize(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_SECRETKEYBYTES
    );

    int result =
        PQCLEAN_FALCON512_CLEAN_crypto_sign_keypair_seeded(
            publicKey.data(),
            privateKey.data(),
            seed.data()
        );

    return result == 0;
}

bool Falcon512_Sign(
    const std::vector<unsigned char>& privateKey,
    const std::vector<unsigned char>& message,
    std::vector<unsigned char>& signature
)
{

    signature.resize(
        PQCLEAN_FALCON512_CLEAN_CRYPTO_BYTES
    );


    size_t siglen = 0;


    int result =
    PQCLEAN_FALCON512_CLEAN_crypto_sign_signature(
        signature.data(),
        &siglen,
        message.data(),
        message.size(),
        privateKey.data()
    );


    if(result != 0)
        return false;


    signature.resize(siglen);


    return true;
}



bool Falcon512_Verify(
    const std::vector<unsigned char>& publicKey,
    const std::vector<unsigned char>& message,
    const std::vector<unsigned char>& signature
)
{

    int result =
    PQCLEAN_FALCON512_CLEAN_crypto_sign_verify(
        signature.data(),
        signature.size(),
        message.data(),
        message.size(),
        publicKey.data()
    );


    return result == 0;
}
