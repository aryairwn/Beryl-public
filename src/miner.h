// © Arya Irawan — 10 August 2026

#ifndef BERYL_MINER_H
#define BERYL_MINER_H

#include "block.h"
#include "chain.h"

class Mempool;

bool CreateBlockTemplate(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
);

bool MineBlock(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
);

uint64_t GetMiningHashrate();
uint64_t GetMiningHashAttempts();
int GetMiningWorkers();

#endif



