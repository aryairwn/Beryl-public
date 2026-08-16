// © Arya Irawan — 10 August 2026

#ifndef BERYL_UTXO_MANAGER_H
#define BERYL_UTXO_MANAGER_H

#include "utxo.h"
#include "block.h"

#include <vector>

class UTXOManager
{

private:

    UTXOSet utxoSet;

public:

    void ProcessBlock(
        const BerylBlock& block
    );

void Rebuild(
    const std::vector<BerylBlock>& blocks
);

    uint64_t GetBalance(
        const std::string& address
    ) const;

    // Akses read-only untuk validasi blockchain.
    // Caller tidak dapat mengubah UTXOSet melalui reference ini.
    const UTXOSet& GetUTXOSet() const;

    UTXOSet& GetUTXOSetMutable();

    // Seluruh UTXO aktif untuk deterministic commitment.
    const std::vector<UTXO>& GetAllUTXOs() const
    {
        return utxoSet.GetAll();
    }

};

#endif
