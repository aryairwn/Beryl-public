// © Arya Irawan — 10 August 2026

#include "mempool.h"

bool Mempool::Contains(
    const std::string& txid
) const
{
    for (const auto& tx : transactions)
    {
        if (tx.txid == txid)
            return true;
    }

    return false;
}

bool Mempool::AddTransaction(
    const UTXOSet& utxos,
    const BerylTransaction& tx,
    int currentHeight
)
{
    // Coinbase tidak boleh masuk mempool.
    if (tx.vin.empty())
        return false;

    // TXID wajib ada.
    if (tx.txid.empty())
        return false;

    // Jangan menerima transaksi yang sama dua kali.
    if (Contains(tx.txid))
        return false;

    // --------------------------------------------------------
    // Cegah double-spend antar transaksi di mempool.
    //
    // Setiap input TX baru tidak boleh memakai UTXO
    // yang sudah dipakai oleh transaksi lain di mempool.
    // --------------------------------------------------------

    for (const auto& existingTx : transactions)
    {
        for (const auto& existingInput : existingTx.vin)
        {
            for (const auto& newInput : tx.vin)
            {
                if (
                    existingInput.previousTx ==
                        newInput.previousTx &&
                    existingInput.outputIndex ==
                        newInput.outputIndex
                )
                {
                    return false;
                }
            }
        }
    }

    // Validasi transaksi.
    if (!ValidateTransaction(
            utxos,
            tx,
            currentHeight
        ))
        return false;

    // Fee wajib memenuhi minimum.
    const uint64_t fee =
        CalculateTransactionFee(
            utxos,
            tx
        );

    if (fee < MIN_TRANSACTION_FEE)
        return false;

    transactions.push_back(tx);

    return true;
}

bool Mempool::RemoveTransaction(
    const std::string& txid
)
{
    for (auto it = transactions.begin();
         it != transactions.end();
         ++it)
    {
        if (it->txid == txid)
        {
            transactions.erase(it);
            return true;
        }
    }

    return false;
}

void Mempool::RemoveTransactions(
    const std::vector<BerylTransaction>& blockTransactions
)
{
    for (const auto& tx : blockTransactions)
    {
        // Coinbase tidak pernah berada di mempool.
        if (tx.vin.empty())
            continue;

        RemoveTransaction(tx.txid);
    }
}

const std::vector<BerylTransaction>&
Mempool::GetTransactions() const
{
    return transactions;
}

uint64_t Mempool::GetTotalFees(
    const UTXOSet& utxos
) const
{
    uint64_t totalFees = 0;

    for (const auto& tx : transactions)
    {
        const uint64_t fee =
            CalculateTransactionFee(
                utxos,
                tx
            );

        if (fee == 0)
            continue;

        if (UINT64_MAX - totalFees < fee)
            return 0;

        totalFees += fee;
    }

    return totalFees;
}

size_t Mempool::Size() const
{
    return transactions.size();
}

void Mempool::Clear()
{
    transactions.clear();
}
