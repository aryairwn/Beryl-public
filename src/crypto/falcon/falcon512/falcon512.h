#ifndef BERYL_FALCON512_H
#define BERYL_FALCON512_H

#include <vector>

bool Falcon512_KeyGen(
    std::vector<unsigned char>& publicKey,
    std::vector<unsigned char>& privateKey
);

bool Falcon512_KeyGenFromSeed(
    const std::vector<unsigned char>& seed,
    std::vector<unsigned char>& publicKey,
    std::vector<unsigned char>& privateKey
);

bool Falcon512_Sign(
    const std::vector<unsigned char>& privateKey,
    const std::vector<unsigned char>& message,
    std::vector<unsigned char>& signature
);

bool Falcon512_Verify(
    const std::vector<unsigned char>& publicKey,
    const std::vector<unsigned char>& message,
    const std::vector<unsigned char>& signature
);

#endif
