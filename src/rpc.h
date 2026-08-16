// © Arya Irawan — 11 August 2026
#ifndef BERYL_RPC_H
#define BERYL_RPC_H

#include <string>
#include "chain.h"
#include "utxomanager.h"
#include "mempool.h"

void StartRPC(
    BerylChain& chain,
    UTXOManager& utxoManager,
    Mempool& mempool,
    const std::string& blockchainFile
);

#endif
