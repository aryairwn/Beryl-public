// © Arya Irawan — 11 August 2026
#ifndef BERYL_BLOCKTEMPLATE_H
#define BERYL_BLOCKTEMPLATE_H

#include <string>

#include "block.h"
#include "chain.h"
#include "mempool.h"

bool CreateBlockTemplate(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
);

#endif
