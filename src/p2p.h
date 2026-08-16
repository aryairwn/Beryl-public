// © Arya Irawan — 10 August 2026

#ifndef BERYL_P2P_H
#define BERYL_P2P_H

#include "chain.h"
#include "utxomanager.h"
#include "mempool.h"

#include <cstdint>
#include <string>

bool IsP2PReady();

void BroadcastBlock(
    const BerylBlock& block
);

void StartP2P(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool
);

// Relay transaksi ke seluruh peer yang sedang terhubung.
void BroadcastTransaction(
    const BerylTransaction& tx
);

#endif

int GetActivePeerCount();

