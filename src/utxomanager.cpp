// © Arya Irawan — 10 August 2026

#include "utxomanager.h"


void UTXOManager::ProcessBlock(
    const BerylBlock& block
)
{
    for (const auto& tx : block.transactions)
    {
        // ========================================================
        // 1. SPEND UTXO INPUT
        // ========================================================
        // Coinbase tidak mempunyai input.
        // Jadi hanya transaksi biasa yang mempunyai UTXO
        // untuk dihabiskan.
        for (const auto& in : tx.vin)
        {
            if (!in.previousTx.empty())
            {
                utxoSet.Spend(
                    in.previousTx,
                    in.outputIndex
                );
            }
        }

        // ========================================================
        // 2. CREATE UTXO OUTPUT
        // ========================================================
        for (uint32_t i = 0;
             i < tx.vout.size();
             ++i)
        {
            UTXO out;

            out.txid = tx.txid;
            out.index = i;
            out.address = tx.vout[i].address;
            out.amount = tx.vout[i].amount;

// Metadata untuk coinbase maturity.
out.height = block.header.height;
out.coinbase = tx.vin.empty();

            utxoSet.Add(out);
        }
    }
}

void UTXOManager::Rebuild(
    const std::vector<BerylBlock>& blocks
)
{

    for(const auto& block : blocks)
    {
        ProcessBlock(block);
    }

}



uint64_t UTXOManager::GetBalance(
    const std::string& address
) const
{

    return utxoSet.GetBalance(
        address
    );

}


const UTXOSet& UTXOManager::GetUTXOSet() const
{
    return utxoSet;
}

UTXOSet& UTXOManager::GetUTXOSetMutable()
{
    return utxoSet;
}
