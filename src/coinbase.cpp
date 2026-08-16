// © Arya Irawan — 10 August 2026

#include "coinbase.h"
#include "consensus.h"

BerylTransaction CreateCoinbaseTransaction(
    const std::string& minerAddress,
    uint64_t reward,
    const std::string& coinbaseData
)
{
    BerylTransaction tx;

    // Data unik coinbase.
    // Data ini ikut dihitung ke TXID.
    tx.coinbaseData = coinbaseData;

    // Coinbase tidak punya input
    tx.vin.clear();

    TxOutput out;

    out.address = minerAddress;
    out.amount = reward;

    tx.vout.push_back(out);

    tx.txid = CalculateTxID(tx);

    return tx;
}


// ============================================================
// CALCULATE COINBASE REWARD
// ============================================================

uint64_t CalculateCoinbaseReward(
    int height,
    const UTXOSet& utxos,
    const std::vector<BerylTransaction>& transactions
)
{
    if (height < 0)
        return 0;

    // Subsidy berasal dari aturan konsensus Beryl.
    uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(height);

    // Hitung seluruh fee transaksi dalam block.
    uint64_t fees =
        CalculateBlockFees(utxos, transactions);

    // Cegah overflow uint64_t.
    if (UINT64_MAX - subsidy < fees)
        return 0;

    return subsidy + fees;
}
