// © Arya Irawan — 10 August 2026

#include "wallet.h"
#include "crypto/blake3_wrapper.h"
#include <sstream>

#include <fstream>
#include <vector>

bool BerylWallet::Generate()
{
    // Generate cryptographically secure 256-bit entropy.
    std::vector<unsigned char> entropy;

    if (!BerylSeed::GenerateEntropy(entropy))
        return false;

    // Convert entropy into the permanent 24-word Beryl Seed V1.
    std::vector<std::string> words;

    if (!BerylSeed::EntropyToMnemonic(entropy, words))
        return false;

    return GenerateFromMnemonic(words);
}

// ============================================================
// GENERATE WALLET FROM BERYL SEED V1
// ============================================================

bool BerylWallet::GenerateFromMnemonic(
    const std::vector<std::string>& words
)
{
    std::vector<unsigned char> entropy;

    if (!BerylSeed::MnemonicToEntropy(words, entropy))
        return false;

    std::vector<unsigned char> derivedMaster;

    if (!BerylSeed::DeriveMasterSeed(
            entropy,
            derivedMaster))
    {
        return false;
    }

    std::vector<unsigned char> keySeed;

    if (!BerylSeed::DeriveKeySeed(
            derivedMaster,
            0,
            keySeed))
    {
        return false;
    }

    FalconKey newKey;

    if (!newKey.GenerateFromSeed(keySeed))
        return false;

    key = newKey;
    masterSeed = derivedMaster;
    seedWords = words;

    // Derive the Beryl address from the Falcon public key.
    std::string pubHex = key.GetPublicKeyHex();

    std::vector<unsigned char> pub(
        pubHex.begin(),
        pubHex.end()
    );

    std::vector<unsigned char> hash =
        Blake3Hash(pub, 20);

    static const char hex[] =
        "0123456789abcdef";

    address = "ber";

    for (unsigned char b : hash)
    {
        address += hex[(b >> 4) & 0x0F];
        address += hex[b & 0x0F];
    }

    additionalKeys.clear();

    return true;
}

// ============================================================
// RESTORE WALLET FROM BERYL SEED V1
// ============================================================

bool BerylWallet::RestoreFromMnemonic(
    const std::vector<std::string>& words
)
{
    return GenerateFromMnemonic(words);
}

// ============================================================
// SEED PHRASE
// ============================================================

std::string BerylWallet::GetSeedPhrase() const
{
    return BerylSeed::MnemonicToString(seedWords);
}

// ============================================================
// GENERATE NEW ADDRESS
// ============================================================

bool BerylWallet::GenerateNewAddress(
    std::string& newAddress
)
{
    if (masterSeed.size() != 32)
        return false;

    // Address index 0 belongs to the main wallet.
    // Additional addresses start from index 1.
    uint32_t index =
        static_cast<uint32_t>(additionalKeys.size()) + 1;

    std::vector<unsigned char> keySeed;

    if (!BerylSeed::DeriveKeySeed(
            masterSeed,
            index,
            keySeed))
    {
        return false;
    }

    WalletKey newEntry;

    if (!newEntry.key.GenerateFromSeed(keySeed))
        return false;

    std::string pubHex =
        newEntry.key.GetPublicKeyHex();

    std::vector<unsigned char> pub(
        pubHex.begin(),
        pubHex.end()
    );

    std::vector<unsigned char> hash =
        Blake3Hash(pub, 20);

    static const char hex[] =
        "0123456789abcdef";

    newEntry.address = "ber";

    for (unsigned char b : hash)
    {
        newEntry.address +=
            hex[(b >> 4) & 0x0F];

        newEntry.address +=
            hex[b & 0x0F];
    }

    newAddress = newEntry.address;

    additionalKeys.push_back(newEntry);

    return true;
}


// ============================================================
// RESTORE DETERMINISTIC ADDRESS BY INDEX
// ============================================================

bool BerylWallet::RestoreAddressIndex(
    uint32_t index
)
{
    // Index 0 adalah main wallet dan sudah dibuat
    // oleh GenerateFromMnemonic().
    if (index == 0)
        return true;

    if (masterSeed.size() != 32)
        return false;

    // Jangan masukkan address yang sama dua kali.
    if (index <= additionalKeys.size())
    {
        return true;
    }

    std::vector<unsigned char> keySeed;

    if (!BerylSeed::DeriveKeySeed(
            masterSeed,
            index,
            keySeed))
    {
        return false;
    }

    WalletKey newEntry;

    if (!newEntry.key.GenerateFromSeed(keySeed))
        return false;

    const std::string pubHex =
        newEntry.key.GetPublicKeyHex();

    std::vector<unsigned char> pub(
        pubHex.begin(),
        pubHex.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(pub, 20);

    static const char hex[] =
        "0123456789abcdef";

    newEntry.address = "ber";

    for (unsigned char b : hash)
    {
        newEntry.address +=
            hex[(b >> 4) & 0x0F];

        newEntry.address +=
            hex[b & 0x0F];
    }

    // Jika index harus berurutan, tambahkan sampai index tersebut.
    // Fungsi ini digunakan oleh recovery, sehingga gap index
    // tetap dibuat secara deterministik.
    while (additionalKeys.size() < index)
    {
        uint32_t nextIndex =
            static_cast<uint32_t>(additionalKeys.size()) + 1;

        if (nextIndex == index)
        {
            additionalKeys.push_back(newEntry);
            return true;
        }

        std::vector<unsigned char> nextSeed;

        if (!BerylSeed::DeriveKeySeed(
                masterSeed,
                nextIndex,
                nextSeed))
        {
            return false;
        }

        WalletKey entry;

        if (!entry.key.GenerateFromSeed(nextSeed))
            return false;

        const std::string nextPubHex =
            entry.key.GetPublicKeyHex();

        std::vector<unsigned char> nextPub(
            nextPubHex.begin(),
            nextPubHex.end()
        );

        const std::vector<unsigned char> nextHash =
            Blake3Hash(nextPub, 20);

        entry.address = "ber";

        for (unsigned char b : nextHash)
        {
            entry.address +=
                hex[(b >> 4) & 0x0F];

            entry.address +=
                hex[b & 0x0F];
        }

        additionalKeys.push_back(entry);
    }

    return true;
}

bool BerylWallet::Save(
    const std::string& path
) const
{
    std::ofstream file(path);

    if(!file)
    {
        return false;
    }

    // ============================================================
    // BERYL SEED V1
    // ============================================================

    file
        << "version=2"
        << "\n";

    file
        << "seed_phrase="
        << GetSeedPhrase()
        << "\n";

    // ============================================================
    // MAIN WALLET
    // ============================================================

    file
        << "address="
        << address
        << "\n";

        file
            << "hidden="
            << (hidden ? "1" : "0")
            << "\n";

    file
        << "private_key="
        << key.GetPrivateKeyHex()
        << "\n";

    file
        << "public_key="
        << key.GetPublicKeyHex()
        << "\n";

    // ============================================================
    // ADDITIONAL ADDRESSES
    // ============================================================

    for(const auto& entry : additionalKeys)
    {
        file
            << "additional_address="
            << entry.address
            << "\n";

            file
                << "additional_hidden="
                << (entry.hidden ? "1" : "0")
                << "\n";

        file
            << "additional_private_key="
            << entry.key.GetPrivateKeyHex()
            << "\n";

        file
            << "additional_public_key="
            << entry.key.GetPublicKeyHex()
            << "\n";
    }

    file.close();

    return true;
}

bool BerylWallet::Load(
    const std::string& path
)
{
    std::ifstream file(path);

    if(!file)
        return false;

    std::string line;
    std::string seedPhrase;

    std::string storedAddress;
    std::string storedPublicKeyHex;
    std::string storedPrivateKeyHex;
    bool storedHidden = false;


    std::vector<std::string> storedAdditionalAddresses;
    std::vector<bool> storedAdditionalHidden;
    std::vector<std::string> storedAdditionalPublicKeys;
    std::vector<std::string> storedAdditionalPrivateKeys;

    while(std::getline(file, line))
    {
        if(line.rfind("seed_phrase=", 0) == 0)
        {
            seedPhrase = line.substr(12);
        }
        else if(line.rfind("address=", 0) == 0)
        {
            storedAddress = line.substr(8);
        }
        else if(line.rfind("hidden=", 0) == 0)
        {
            storedHidden = (line.substr(7) == "1");
        }
        else if(line.rfind("public_key=", 0) == 0)
        {
            storedPublicKeyHex = line.substr(11);
        }
        else if(line.rfind("private_key=", 0) == 0)
        {
            storedPrivateKeyHex = line.substr(12);
        }
        else if(line.rfind("additional_address=", 0) == 0)
        {
            storedAdditionalAddresses.push_back(line.substr(19));
        }
        else if(line.rfind("additional_hidden=", 0) == 0)
        {
            storedAdditionalHidden.push_back(
                line.substr(18) == "1"
            );
        }
        else if(line.rfind("additional_public_key=", 0) == 0)
        {
            storedAdditionalPublicKeys.push_back(line.substr(22));
        }
        else if(line.rfind("additional_private_key=", 0) == 0)
        {
            storedAdditionalPrivateKeys.push_back(line.substr(23));
        }
    }

    file.close();

    // ============================================================
    // BERYL SEED V1 RECOVERY
    // ============================================================

    if(seedPhrase.empty())
    {
        // Legacy wallet without seed phrase.
        // Keep compatibility with the old private/public-key format.
        if(storedAddress.empty() ||
           storedPublicKeyHex.empty() ||
           storedPrivateKeyHex.empty())
        {
            return false;
        }

        auto HexToBytes =
            [](const std::string& hex,
               std::vector<unsigned char>& out) -> bool
        {
            if(hex.empty() || (hex.size() % 2) != 0)
                return false;

            out.clear();
            out.reserve(hex.size() / 2);

            auto HexValue =
                [](char c) -> int
            {
                if(c >= '0' && c <= '9')
                    return c - '0';

                if(c >= 'a' && c <= 'f')
                    return c - 'a' + 10;

                if(c >= 'A' && c <= 'F')
                    return c - 'A' + 10;

                return -1;
            };

            for(size_t i = 0; i < hex.size(); i += 2)
            {
                int hi = HexValue(hex[i]);
                int lo = HexValue(hex[i + 1]);

                if(hi < 0 || lo < 0)
                    return false;

                out.push_back(
                    static_cast<unsigned char>((hi << 4) | lo)
                );
            }

            return true;
        };

        std::vector<unsigned char> pub;
        std::vector<unsigned char> priv;

        if(!HexToBytes(storedPublicKeyHex, pub))
            return false;

        if(!HexToBytes(storedPrivateKeyHex, priv))
            return false;

        if(!key.SetKeys(pub, priv))
            return false;

        address = storedAddress;

        return true;
    }

    // Parse 24-word Beryl Seed V1.
    std::vector<std::string> words;

    if(!BerylSeed::StringToMnemonic(
            seedPhrase,
            words))
    {
        return false;
    }

    if(!BerylSeed::ValidateMnemonic(words))
        return false;

    // Recreate wallet deterministically from the seed.
    if(!GenerateFromMnemonic(words))
        return false;

    // ============================================================
    // COMPATIBILITY CHECK
    //
    // If an old seed-enabled wallet contains stored keys,
    // verify that deterministic recovery produces exactly
    // the same public/private key and address.
    // ============================================================

    if(!storedAddress.empty() &&
       storedAddress != address)
    {
        return false;
    }

    if(!storedPublicKeyHex.empty() &&
       storedPublicKeyHex != key.GetPublicKeyHex())
    {
        return false;
    }

    if(!storedPrivateKeyHex.empty() &&
       storedPrivateKeyHex != key.GetPrivateKeyHex())
    {
        return false;
    }

    // ============================================================
    // RECOVER ADDITIONAL ADDRESSES
    // ============================================================

    if(storedAdditionalAddresses.size() !=
       storedAdditionalPublicKeys.size())
    {
        return false;
    }

    if(storedAdditionalAddresses.size() !=
       storedAdditionalPrivateKeys.size())
    {
        return false;
    }

    // Kompatibilitas wallet lama:
    // sebelum fitur hidden, wallet.dat tidak memiliki
    // additional_hidden=. Semua address lama dianggap visible.
    if(storedAdditionalHidden.empty())
    {
        storedAdditionalHidden.resize(
            storedAdditionalAddresses.size(),
            false
        );
    }

    if(storedAdditionalHidden.size() !=
       storedAdditionalAddresses.size())
    {
        return false;
    }

    additionalKeys.clear();

    for(size_t i = 0;
        i < storedAdditionalAddresses.size();
        ++i)
    {
        uint32_t index =
            static_cast<uint32_t>(i) + 1;

        std::vector<unsigned char> keySeed;

        if(!BerylSeed::DeriveKeySeed(
                masterSeed,
                index,
                keySeed))
        {
            return false;
        }

        WalletKey entry;

        if(!entry.key.GenerateFromSeed(keySeed))
            return false;

        std::string derivedAddress;

        std::string pubHex =
            entry.key.GetPublicKeyHex();

        std::vector<unsigned char> pub(
            pubHex.begin(),
            pubHex.end()
        );

        std::vector<unsigned char> hash =
            Blake3Hash(pub, 20);

        static const char hex[] =
            "0123456789abcdef";

        derivedAddress = "ber";

        for(unsigned char b : hash)
        {
            derivedAddress +=
                hex[(b >> 4) & 0x0F];

            derivedAddress +=
                hex[b & 0x0F];
        }

        if(storedAdditionalAddresses[i] !=
           derivedAddress)
        {
            return false;
        }

        if(storedAdditionalPublicKeys[i] !=
           entry.key.GetPublicKeyHex())
        {
            return false;
        }

        if(storedAdditionalPrivateKeys[i] !=
           entry.key.GetPrivateKeyHex())
        {
            return false;
        }

        entry.address = derivedAddress;
        entry.hidden = storedAdditionalHidden[i];

        additionalKeys.push_back(entry);
    }

    return true;
}

std::string BerylWallet::GetPublicKeyHex() const
{
    return key.GetPublicKeyHex();
}


std::string BerylWallet::GetPrivateKeyHex() const
{
    return key.GetPrivateKeyHex();
}

// ============================================================
// GET ALL ADDRESSES
// ============================================================

std::string BerylWallet::GetAddress() const
{
    return address;
}

std::vector<std::string> BerylWallet::GetAddresses() const
{
    std::vector<std::string> result;

    // Address hidden tetap tersimpan di wallet,
    // tetapi tidak ditampilkan ke daftar address aktif.
    if(!address.empty() && !hidden)
    {
        result.push_back(address);
    }

    for(const auto& entry : additionalKeys)
    {
        if(!entry.address.empty() && !entry.hidden)
        {
            result.push_back(entry.address);
        }
    }

    return result;
}

// ============================================================
// HIDE / UNHIDE ADDRESS
// ============================================================

bool BerylWallet::HideAddress(const std::string& targetAddress)
{
    if(targetAddress == address)
    {
        hidden = true;
        return true;
    }

    for(auto& entry : additionalKeys)
    {
        if(entry.address == targetAddress)
        {
            entry.hidden = true;
            return true;
        }
    }

    return false;
}

bool BerylWallet::UnhideAddress(const std::string& targetAddress)
{
    if(targetAddress == address)
    {
        hidden = false;
        return true;
    }

    for(auto& entry : additionalKeys)
    {
        if(entry.address == targetAddress)
        {
            entry.hidden = false;
            return true;
        }
    }

    return false;
}

bool BerylWallet::IsAddressHidden(
    const std::string& targetAddress
) const
{
    if(targetAddress == address)
        return hidden;

    for(const auto& entry : additionalKeys)
    {
        if(entry.address == targetAddress)
            return entry.hidden;
    }

    return false;
}

// ============================================================
// GET FALCON KEY BY ADDRESS
// ============================================================

bool BerylWallet::GetKeyForAddress(
    const std::string& targetAddress,
    FalconKey& outKey
) const
{
    // Main wallet address.
    if (targetAddress == address)
    {
        std::vector<unsigned char> pub;
        std::vector<unsigned char> priv;

        const std::string pubHex = key.GetPublicKeyHex();
        const std::string privHex = key.GetPrivateKeyHex();

        if (pubHex.empty() || privHex.empty())
            return false;

        auto HexToBytes = [](
            const std::string& hex,
            std::vector<unsigned char>& data
        ) -> bool
        {
            if (hex.size() % 2 != 0)
                return false;

            data.clear();
            data.reserve(hex.size() / 2);

            for (size_t i = 0; i < hex.size(); i += 2)
            {
                unsigned int value = 0;

                std::stringstream ss;
                ss << std::hex << hex.substr(i, 2);
                ss >> value;

                if (ss.fail())
                    return false;

                data.push_back(
                    static_cast<unsigned char>(value)
                );
            }

            return true;
        };

        if (!HexToBytes(pubHex, pub))
            return false;

        if (!HexToBytes(privHex, priv))
            return false;

        return outKey.SetKeys(pub, priv);
    }

    // Additional wallet addresses.
    for (const auto& entry : additionalKeys)
    {
        if (entry.address == targetAddress)
        {
            return outKey.SetKeys(
                [&]() {
                    std::vector<unsigned char> v;
                    const std::string h = entry.key.GetPublicKeyHex();

                    if (h.size() % 2 != 0)
                        return v;

                    v.reserve(h.size() / 2);

                    for (size_t i = 0; i < h.size(); i += 2)
                    {
                        unsigned int x = 0;
                        std::stringstream ss;
                        ss << std::hex << h.substr(i, 2);
                        ss >> x;

                        if (ss.fail())
                        {
                            v.clear();
                            return v;
                        }

                        v.push_back(
                            static_cast<unsigned char>(x)
                        );
                    }

                    return v;
                }(),
                [&]() {
                    std::vector<unsigned char> v;
                    const std::string h = entry.key.GetPrivateKeyHex();

                    if (h.size() % 2 != 0)
                        return v;

                    v.reserve(h.size() / 2);

                    for (size_t i = 0; i < h.size(); i += 2)
                    {
                        unsigned int x = 0;
                        std::stringstream ss;
                        ss << std::hex << h.substr(i, 2);
                        ss >> x;

                        if (ss.fail())
                        {
                            v.clear();
                            return v;
                        }

                        v.push_back(
                            static_cast<unsigned char>(x)
                        );
                    }

                    return v;
                }()
            );
        }
    }

    return false;
}

