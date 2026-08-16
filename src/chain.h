// © Arya Irawan — 10 August 2026

#ifndef BERYL_CHAIN_H
#define BERYL_CHAIN_H

#include "block.h"
#include "contract_state_manager.h"
#include <vector>

class UTXOManager;

class BerylChain
{

private:

    std::vector<BerylBlock> blocks;

    UTXOManager* utxoManager = nullptr;

    // Persistent contract state belonging to this chain.
    beryl::contract::ContractStateManager contractState;

public:

void SetUTXOManager(
    UTXOManager* manager
);

    const UTXOManager* GetUTXOManager() const;

    beryl::contract::ContractStateManager& GetContractStateManager();
    const beryl::contract::ContractStateManager& GetContractStateManager() const;


    bool AddBlock(
        const BerylBlock& block
    );

    // Internal: memasukkan block hasil persistence
    // tanpa memproses UTXO. UTXO akan direbuild
    // setelah seluruh blockchain selesai dimuat.
    bool LoadBlock(
        const BerylBlock& block
    );

const BerylBlock& GetLastBlock() const;

    std::string GetLastHash() const;


    int GetHeight() const;


    const std::vector<BerylBlock>& GetBlocks() const;

const BerylBlock& GetBlock(
    int index
) const;

};



#endif
