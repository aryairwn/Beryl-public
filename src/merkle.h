// © Arya Irawan — 10 August 2026

#ifndef BERYL_MERKLE_H
#define BERYL_MERKLE_H

#include <string>
#include <vector>

#include "transaction.h"

std::string HashPair(
    const std::string& left,
    const std::string& right
);

std::string CalculateMerkleRoot(
    const std::vector<BerylTransaction>& txs
);

#endif
