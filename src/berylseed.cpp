// © Arya Irawan — 10 August 2026

#include "berylseed.h"
#include "beryl_wordlist.h"
#include "crypto/blake3_wrapper.h"

extern "C" {
#include "crypto/BLAKE3/c/blake3.h"
}

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

extern "C"
{
#include "crypto/falcon/PQClean/common/randombytes.h"
}

namespace
{

// ============================================================
// BERYL WORDLIST V1
//
// 4096 immutable English words = 2^12.
// Each word represents exactly 12 bits.
//
// The permanent wordlist is stored in beryl_wordlist.h.
// DO NOT reorder, remove, or modify entries after V1 launch.
// ============================================================

static std::string WordForIndex(uint16_t index)
{
    if (index >= BerylWordlist::SIZE)
        return "";

    return BerylWordlist::WORDS[index];
}

static bool IndexForWord(
    const std::string& word,
    uint16_t& index)
{
    for (std::size_t i = 0; i < BerylWordlist::SIZE; ++i)
    {
        if (word == BerylWordlist::WORDS[i])
        {
            index = static_cast<uint16_t>(i);
            return true;
        }
    }

    return false;
}

// ============================================================
// Convert bytes -> 288 bits

// ============================================================

static void BytesToBits(
    const std::vector<unsigned char>& data,
    std::vector<unsigned char>& bits)
{
    bits.clear();
    bits.reserve(data.size() * 8);

    for (unsigned char b : data)
    {
        for (int i = 7; i >= 0; --i)
        {
            bits.push_back(
                static_cast<unsigned char>((b >> i) & 1)
            );
        }
    }
}

// ============================================================
// Convert bits -> bytes
// ============================================================

static bool BitsToBytes(
    const std::vector<unsigned char>& bits,
    std::vector<unsigned char>& data)
{
    if ((bits.size() % 8) != 0)
        return false;

    data.assign(bits.size() / 8, 0);

    for (size_t i = 0; i < bits.size(); ++i)
    {
        data[i / 8] <<= 1;
        data[i / 8] |= bits[i] & 1;
    }

    return true;
}

// ============================================================
// Read 12-bit word index
// ============================================================

static uint16_t Read12(
    const std::vector<unsigned char>& bits,
    size_t offset)
{
    uint16_t value = 0;

    for (size_t i = 0; i < 12; ++i)
    {
        value <<= 1;
        value |= bits[offset + i] & 1;
    }

    return value;
}

}

// ============================================================
// Generate 256-bit entropy
// ============================================================

bool BerylSeed::GenerateEntropy(
    std::vector<unsigned char>& entropy)
{
    entropy.resize(ENTROPY_BYTES);

    return randombytes(
        entropy.data(),
        entropy.size()
    ) == 0;
}

// ============================================================
// Entropy -> 24 words
//
// 256-bit entropy
// + 32-bit BLAKE3 checksum
// = 288 bits
// = 24 × 12-bit indexes
// ============================================================

bool BerylSeed::EntropyToMnemonic(
    const std::vector<unsigned char>& entropy,
    std::vector<std::string>& words)
{
    if (entropy.size() != ENTROPY_BYTES)
        return false;

    // 256-bit entropy + 32-bit BLAKE3 checksum = 288 bits.
    std::vector<unsigned char> checksumInput = entropy;

    std::vector<unsigned char> hash =
        Blake3Hash(entropy, 32);

    checksumInput.push_back(hash[0]);
    checksumInput.push_back(hash[1]);
    checksumInput.push_back(hash[2]);
    checksumInput.push_back(hash[3]);

    std::vector<unsigned char> bits;
    BytesToBits(checksumInput, bits);

    if (bits.size() != 288)
        return false;

    words.clear();
    words.reserve(WORD_COUNT);

    for (size_t i = 0; i < WORD_COUNT; ++i)
    {
        uint16_t index =
            Read12(bits, i * WORD_BITS);

        if (index >= BerylWordlist::SIZE)
            return false;

        std::string word = WordForIndex(index);

        if (word.empty())
            return false;

        words.push_back(word);
    }

    return true;
}

// ============================================================
// Mnemonic -> entropy
// ============================================================

bool BerylSeed::MnemonicToEntropy(
    const std::vector<std::string>& words,
    std::vector<unsigned char>& entropy)
{
    if (words.size() != WORD_COUNT)
        return false;

    std::vector<unsigned char> bits;
    bits.reserve(288);

    for (const auto& word : words)
    {
        uint16_t index = 0;

        if (!IndexForWord(word, index))
            return false;

        for (int i = 11; i >= 0; --i)
        {
            bits.push_back(
                static_cast<unsigned char>(
                    (index >> i) & 1
                )
            );
        }
    }

    if (bits.size() != 288)
        return false;

    std::vector<unsigned char> data;

    if (!BitsToBytes(bits, data))
        return false;

    if (data.size() != SEED_DATA_BYTES)
        return false;

    entropy.assign(
        data.begin(),
        data.begin() + ENTROPY_BYTES
    );

    // 32-bit BLAKE3 checksum.
    std::vector<unsigned char> hash =
        Blake3Hash(entropy, 32);

    if (data[32] != hash[0] ||
        data[33] != hash[1] ||
        data[34] != hash[2] ||
        data[35] != hash[3])
    {
        entropy.clear();
        return false;
    }

    return true;
}

// ============================================================
// Validate
// ============================================================

bool BerylSeed::ValidateMnemonic(
    const std::vector<std::string>& words)
{
    std::vector<unsigned char> entropy;

    return MnemonicToEntropy(words, entropy);
}

// ============================================================
// Beryl Seed V1 master seed
//
// 256-bit entropy -> 256-bit master seed
//
// BLAKE3 derive-key context provides domain separation.
// ============================================================

bool BerylSeed::DeriveMasterSeed(
    const std::vector<unsigned char>& entropy,
    std::vector<unsigned char>& masterSeed)
{
    if (entropy.size() != ENTROPY_BYTES)
        return false;

    masterSeed.resize(32);

    blake3_hasher hasher;

    blake3_hasher_init_derive_key(
        &hasher,
        "Beryl Seed V1 Master"
    );

    blake3_hasher_update(
        &hasher,
        entropy.data(),
        entropy.size()
    );

    blake3_hasher_finalize(
        &hasher,
        masterSeed.data(),
        masterSeed.size()
    );

    return true;
}

// ============================================================
// Deterministic key seed
// ============================================================

bool BerylSeed::DeriveKeySeed(
    const std::vector<unsigned char>& masterSeed,
    uint32_t index,
    std::vector<unsigned char>& keySeed)
{
    if (masterSeed.size() != 32)
        return false;

    // Falcon-512 seeded key generation requires exactly 48 bytes.
    keySeed.resize(BerylSeed::FALCON_SEED_BYTES);

    blake3_hasher hasher;

    blake3_hasher_init_keyed(
        &hasher,
        masterSeed.data()
    );

    static const unsigned char domain[] =
        "BERYL-FALCON-512-V1";

    blake3_hasher_update(
        &hasher,
        domain,
        sizeof(domain) - 1
    );

    unsigned char indexBytes[4];

    indexBytes[0] =
        static_cast<unsigned char>(index & 0xff);

    indexBytes[1] =
        static_cast<unsigned char>((index >> 8) & 0xff);

    indexBytes[2] =
        static_cast<unsigned char>((index >> 16) & 0xff);

    indexBytes[3] =
        static_cast<unsigned char>((index >> 24) & 0xff);

    blake3_hasher_update(
        &hasher,
        indexBytes,
        sizeof(indexBytes)
    );

    blake3_hasher_finalize(
        &hasher,
        keySeed.data(),
        keySeed.size()
    );

    return true;
}

// ============================================================
// Deterministic Falcon-512 seed
//
// Beryl Seed V1
// master seed + address index
// -> BLAKE3 keyed derivation
// -> 48 bytes
//
// 48 bytes = exact seed size required by the
// Beryl Falcon-512 seeded key generator.
// ============================================================

bool BerylSeed::DeriveFalconSeed(
    const std::vector<unsigned char>& masterSeed,
    uint32_t index,
    std::vector<unsigned char>& falconSeed
)
{
    if (masterSeed.size() != 32)
        return false;

    falconSeed.resize(48);

    blake3_hasher hasher;

    blake3_hasher_init_keyed(
        &hasher,
        masterSeed.data()
    );

    static const unsigned char domain[] =
        "BERYL-FALCON-512-SEED-V1";

    blake3_hasher_update(
        &hasher,
        domain,
        sizeof(domain) - 1
    );

    unsigned char indexBytes[4];

    indexBytes[0] =
        static_cast<unsigned char>(index & 0xff);

    indexBytes[1] =
        static_cast<unsigned char>((index >> 8) & 0xff);

    indexBytes[2] =
        static_cast<unsigned char>((index >> 16) & 0xff);

    indexBytes[3] =
        static_cast<unsigned char>((index >> 24) & 0xff);

    blake3_hasher_update(
        &hasher,
        indexBytes,
        sizeof(indexBytes)
    );

    blake3_hasher_finalize(
        &hasher,
        falconSeed.data(),
        falconSeed.size()
    );

    return true;
}

// ============================================================
// String helpers
// ============================================================

std::string BerylSeed::MnemonicToString(
    const std::vector<std::string>& words)
{
    std::ostringstream out;

    for (size_t i = 0; i < words.size(); ++i)
    {
        if (i != 0)
            out << ' ';

        out << words[i];
    }

    return out.str();
}

bool BerylSeed::StringToMnemonic(
    const std::string& phrase,
    std::vector<std::string>& words)
{
    words.clear();

    std::istringstream in(phrase);
    std::string word;

    while (in >> word)
        words.push_back(word);

    return words.size() == WORD_COUNT;
}
