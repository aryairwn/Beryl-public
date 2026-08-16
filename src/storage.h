// © Arya Irawan — 10 August 2026

#ifndef BERYL_STORAGE_H
#define BERYL_STORAGE_H

#include "block.h"
#include "chain.h"
#include "utxomanager.h"

#include <string>

bool SaveBlockchain(
    const BerylChain& chain,
    const std::string& filename
);

bool LoadBlockchain(
    BerylChain& chain,
    UTXOManager& utxoManager,
    const std::string& filename
);

#endif
