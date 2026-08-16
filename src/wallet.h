// © Arya Irawan — 10 August 2026

#ifndef BERYL_WALLET_H
#define BERYL_WALLET_H

#include <string>
#include <vector>
#include "crypto/falcon/falcon.h"
#include "berylseed.h"

struct WalletKey
{
    FalconKey key;
    std::string address;
    bool hidden = false;
};


class BerylWallet
{

private:

    FalconKey key;

    std::string address;
    bool hidden = false;

    std::vector<WalletKey> additionalKeys;

    // Beryl Seed V1 root mnemonic.
    std::vector<std::string> seedWords;

    // 256-bit Beryl Seed V1 master seed.
    std::vector<unsigned char> masterSeed;

public:

    bool Generate();

    // Generate a wallet from a newly-created Beryl Seed V1 mnemonic.
    bool GenerateFromMnemonic(
        const std::vector<std::string>& words
    );

    // Restore wallet from a Beryl Seed V1 mnemonic.
    bool RestoreFromMnemonic(
        const std::vector<std::string>& words
    );

    std::string GetSeedPhrase() const;

    bool Load(
        const std::string& path
    );

    bool Save(
        const std::string& path
    ) const;

    std::string GetAddress() const;

    std::string GetPublicKeyHex() const;

    std::string GetPrivateKeyHex() const;

    // Generate address baru dan simpan keypair di wallet.
    bool GenerateNewAddress(
        std::string& newAddress
    );

    // Restore deterministic address berdasarkan index Beryl Seed V1.
    // Index 0 adalah address utama.
    bool RestoreAddressIndex(
        uint32_t index
    );

    // Semua address yang dimiliki wallet.
    std::vector<std::string> GetAddresses() const;

    // Hide/unhide address tanpa menghapus key.
    // Address hanya boleh di-hide jika saldo = 0.
    bool HideAddress(const std::string& targetAddress);
    bool UnhideAddress(const std::string& targetAddress);
    bool IsAddressHidden(const std::string& targetAddress) const;

    // Mengambil FalconKey berdasarkan address wallet.
    bool GetKeyForAddress(
        const std::string& targetAddress,
        FalconKey& outKey
    ) const;

    // Mengambil semua alamat wallet:
    // address utama + semua address tambahan.

};

#endif
