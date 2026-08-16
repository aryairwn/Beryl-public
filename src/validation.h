// © Arya Irawan — 10 August 2026

#ifndef BERYL_VALIDATION_H
#define BERYL_VALIDATION_H

#include "block.h"


bool CheckBlock(
    const BerylBlock& block,
    const std::string& previousHash
);


#endif
