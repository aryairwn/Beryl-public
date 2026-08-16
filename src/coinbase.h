// © Arya Irawan — 10 August 2026

#ifndef BERYL_COINBASE_H
#define BERYL_COINBASE_H

#include "transaction.h"
#include <string>
#include <cstdint>

BerylTransaction CreateCoinbaseTransaction(
    const std::string& minerAddress,
    uint64_t reward,
    const std::string& coinbaseData = ""
);

// ============================================================
// COINBASE REWARD
// ============================================================

// Total reward yang boleh diklaim miner:
// block subsidy + seluruh transaction fee.
uint64_t CalculateCoinbaseReward(
    int height,
    const UTXOSet& utxos,
    const std::vector<BerylTransaction>& transactions
);

#endif
