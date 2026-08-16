// © Arya Irawan — 10 August 2026

#ifndef BERYL_SEED_H
#define BERYL_SEED_H

#include <string>
#include <vector>
#include <cstdint>

namespace BerylSeed
{
    static constexpr size_t ENTROPY_BYTES = 32;
    static constexpr size_t CHECKSUM_BYTES = 4;
    static constexpr size_t SEED_DATA_BYTES = 36;
    static constexpr size_t WORD_COUNT = 24;
    static constexpr size_t WORD_BITS = 12;
static constexpr size_t FALCON_SEED_BYTES = 48;

    // Generate cryptographically secure 256-bit entropy.
    bool GenerateEntropy(std::vector<unsigned char>& entropy);

    // Convert 256-bit entropy into 24-word Beryl mnemonic.
    bool EntropyToMnemonic(
        const std::vector<unsigned char>& entropy,
        std::vector<std::string>& words
    );

    // Convert 24-word Beryl mnemonic back into 256-bit entropy.
    bool MnemonicToEntropy(
        const std::vector<std::string>& words,
        std::vector<unsigned char>& entropy
    );

    // Validate mnemonic and checksum.
    bool ValidateMnemonic(
        const std::vector<std::string>& words
    );

    // Beryl Seed V1 KDF.
    bool DeriveMasterSeed(
        const std::vector<unsigned char>& entropy,
        std::vector<unsigned char>& masterSeed
    );

    // Derive deterministic key material for address index.
    bool DeriveKeySeed(
        const std::vector<unsigned char>& masterSeed,
        uint32_t index,
        std::vector<unsigned char>& keySeed
    );

    // Derive deterministic 48-byte Falcon-512 seed.
    bool DeriveFalconSeed(
        const std::vector<unsigned char>& masterSeed,
        uint32_t index,
        std::vector<unsigned char>& falconSeed
    );

    // Convert mnemonic to printable seed phrase.
    std::string MnemonicToString(
        const std::vector<std::string>& words
    );

    // Parse printable seed phrase.
    bool StringToMnemonic(
        const std::string& phrase,
        std::vector<std::string>& words
    );
}

#endif
