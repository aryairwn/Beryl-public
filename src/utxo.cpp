// © Arya Irawan — 10 August 2026

#include "utxo.h"

void UTXOSet::Add(
    const UTXO& utxo
)
{
    // Jangan masukkan UTXO yang sama dua kali.
    for (const auto& u : utxos)
    {
        if (u.txid == utxo.txid &&
            u.index == utxo.index)
        {
            return;
        }
    }

    utxos.push_back(utxo);
}

void UTXOSet::Add(
    const std::string& txid,
    uint32_t index,
    const std::string& address,
    uint64_t amount
)
{
    UTXO u;

    u.txid = txid;
    u.index = index;
    u.address = address;
    u.amount = amount;

    Add(u);
}

bool UTXOSet::Spend(
    const std::string& txid,
    uint32_t index
)
{
    for (
        auto it = utxos.begin();
        it != utxos.end();
        ++it
    )
    {
        if (
            it->txid == txid &&
            it->index == index
        )
        {
            utxos.erase(it);
            return true;
        }
    }

    return false;
}

std::vector<UTXO> UTXOSet::GetByAddress(
    const std::string& address
) const
{
    std::vector<UTXO> result;

    for (const auto& u : utxos)
    {
        if (
            address.empty() ||
            u.address == address
        )
        {
            result.push_back(u);
        }
    }

    return result;
}

uint64_t UTXOSet::GetBalance(
    const std::string& address
) const
{
    uint64_t balance = 0;

    for (const auto& u : utxos)
    {
        if (u.address == address)
        {
            balance += u.amount;
        }
    }

    return balance;
}

const std::vector<UTXO>& UTXOSet::GetAll() const
{
    return utxos;
}
