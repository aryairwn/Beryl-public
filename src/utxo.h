// © Arya Irawan — 10 August 2026

#ifndef BERYL_UTXO_H
#define BERYL_UTXO_H

#include <string>
#include <vector>
#include <cstdint>

struct UTXO
{
    std::string txid;
    uint32_t index;

    std::string address;
    uint64_t amount;

    // Tinggi blok saat UTXO dibuat.
    int height = 0;

    // true jika UTXO berasal dari coinbase.
    bool coinbase = false;
};

class UTXOSet
{
public:

    // Tambahkan UTXO lengkap
    void Add(
        const UTXO& utxo
    );

    // Tambahkan UTXO dari transaksi
    void Add(
        const std::string& txid,
        uint32_t index,
        const std::string& address,
        uint64_t amount
    );

    // Habiskan UTXO
    bool Spend(
        const std::string& txid,
        uint32_t index
    );

    // Ambil UTXO berdasarkan address.
    // address kosong = seluruh UTXO.
    std::vector<UTXO> GetByAddress(
        const std::string& address
    ) const;

    // Hitung saldo
    uint64_t GetBalance(
        const std::string& address
    ) const;

    // Seluruh UTXO aktif.
    // Dipakai untuk membuat deterministic UTXO commitment.
    const std::vector<UTXO>& GetAll() const;


private:
    std::vector<UTXO> utxos;
};

#endif
