// © Arya Irawan — 10 August 2026

#ifndef BERYL_DIFFICULTY_H
#define BERYL_DIFFICULTY_H

#include "chain.h"
#include <cstdint>

uint64_t CalculateDifficulty(
    const BerylChain& chain
);

#endif
