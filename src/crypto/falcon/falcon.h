#ifndef BERYL_FALCON_H
#define BERYL_FALCON_H

#include <string>
#include <vector>

class FalconKey
{

private:

    std::vector<unsigned char> publicKey;
    std::vector<unsigned char> privateKey;


public:

    bool Generate();

    // Deterministic Falcon-512 key generation from a 48-byte Beryl seed.
    bool GenerateFromSeed(
        const std::vector<unsigned char>& seed
    );

std::string GetPublicKeyHex() const;
std::string GetPrivateKeyHex() const;

    const std::vector<unsigned char>& GetPublicKeyRaw() const
    {
        return publicKey;
    }


    const std::vector<unsigned char>& GetPrivateKeyRaw() const
    {
        return privateKey;
    }


    bool SetKeys(
        const std::vector<unsigned char>& pub,
        const std::vector<unsigned char>& priv
    );


    std::string Sign(
        const std::string& message
    );


    bool Verify(
        const std::string& message,
        const std::string& signature
    );

};


#endif
