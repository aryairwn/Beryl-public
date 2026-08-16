// © Arya Irawan — 10 August 2026

#ifndef BERYL_BLOCKVALIDATION_H
#define BERYL_BLOCKVALIDATION_H

#include "block.h"
#include "chain.h"
#include "utxomanager.h"

// ============================================================
// BLOCK VALIDATION
// ============================================================
//
// Memeriksa block sebelum masuk ke chain:
//
// 1. Height
// 2. Previous hash
// 3. Merkle root
// 4. Block hash
// 5. Proof-of-Work
// 6. Coinbase
// 7. Semua transaksi
// 8. Coinbase reward + fee
//
// ============================================================

bool ValidateBlock(
    const BerylBlock& block,
    const BerylChain& chain,
    const UTXOManager& utxoManager
);

// ============================================================
// CONTRACT STATE TRANSITION
// ============================================================
//
// Apply satu contract transaction ke candidate/persistent
// ContractStateManager.
//
// Fungsi ini adalah jalur consensus yang sama untuk:
//   - block validation
//   - block application
//
// ============================================================

bool ApplyContractTransactionState(
    beryl::contract::ContractStateManager& contractState,
    const BerylTransaction& tx,
    int height
);

#endif
