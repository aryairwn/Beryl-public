// © Arya Irawan — 10 August 2026

#ifndef BERYL_MEMPOOL_H
#define BERYL_MEMPOOL_H

#include "transaction.h"

#include <vector>
#include <cstdint>

class Mempool
{
private:
    std::vector<BerylTransaction> transactions;

public:
    bool AddTransaction(
        const UTXOSet& utxos,
        const BerylTransaction& tx,
        int currentHeight
    );

    bool RemoveTransaction(
        const std::string& txid
    );

    // Hapus seluruh transaksi yang sudah masuk block.
    void RemoveTransactions(
        const std::vector<BerylTransaction>& transactions
    );

    bool Contains(
        const std::string& txid
    ) const;

    const std::vector<BerylTransaction>&
    GetTransactions() const;

    uint64_t GetTotalFees(
        const UTXOSet& utxos
    ) const;

    size_t Size() const;

    void Clear();
};

#endif
