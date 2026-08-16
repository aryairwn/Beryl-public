// © Arya Irawan — 10 August 2026

#ifndef BERYL_UTXO_ROOT_H
#define BERYL_UTXO_ROOT_H

#include "utxomanager.h"
#include <string>

std::string CalculateUTXORoot(
    const UTXOManager& utxoManager
);

#endif
