// © Arya Irawan — 10 August 2026

#ifndef BERYL_BLOCKHASH_H
#define BERYL_BLOCKHASH_H

#include "block.h"
#include <string>


std::string SerializeHeader(
    const BerylHeader& header
);


std::string GetBlockHash(
    const BerylHeader& header
);


#endif
