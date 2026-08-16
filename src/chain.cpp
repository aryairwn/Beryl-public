// © Arya Irawan — 10 August 2026

#include "chain.h"
#include <iostream>
#include "utxomanager.h"
#include "blockvalidation.h"

bool BerylChain::LoadBlock(
    const BerylBlock& block
)
{
    // Persistence layer sudah membaca block dari
    // file. Pada tahap ini kita hanya memasukkannya
    // ke vector chain.
    //
    // UTXO TIDAK diproses di sini.
    // UTXO akan direbuild oleh LoadBlockchain()
    // setelah seluruh block selesai dimuat.

    if (block.header.height !=
        static_cast<int>(blocks.size() + 1))
    {
        return false;
    }

    blocks.push_back(block);

    return true;
}

bool BerylChain::AddBlock(
    const BerylBlock& block
)
{
    // --------------------------------------------------------
    // 1. UTXO manager wajib tersedia.
    // --------------------------------------------------------

    if (utxoManager == nullptr)
        return false;

    // --------------------------------------------------------
    // 2. Height block harus sesuai posisi berikutnya.
    //
    // Height adalah bagian dari header dan ikut dihitung
    // dalam block hash. Jadi height TIDAK boleh diubah
    // setelah PoW selesai.
    // --------------------------------------------------------

    if (block.header.height !=
        static_cast<int>(blocks.size() + 1))
    {
        return false;
    }

    // --------------------------------------------------------
    // 3. Validasi block sebelum masuk chain.
    // --------------------------------------------------------

    if (!ValidateBlock(
            block,
            *this,
            *utxoManager))
    {
        std::cerr
            << "ADD BLOCK REJECTED: ValidateBlock failed"
            << " HEIGHT="
            << block.header.height
            << " HASH="
            << block.hash
            << "\\n";

        return false;
    }

    // --------------------------------------------------------
    // 4. Block valid -> apply contract state.
    //
    // Contract state harus diperbarui sebelum block masuk
    // ke chain. Jika contract gagal diproses, seluruh block
    // ditolak dan chain tidak berubah.
    // --------------------------------------------------------

    for (size_t i = 1; i < block.transactions.size(); ++i)
    {
        if (!ApplyContractTransactionState(
                contractState,
                block.transactions[i],
                block.header.height))
        {
            std::cerr
                << "ADD BLOCK REJECTED: contract state apply failed"
                << " HEIGHT=" << block.header.height
                << " TXID=" << block.transactions[i].txid
                << "\n";

            return false;
        }
    }

    // --------------------------------------------------------
    // 5. Block valid -> masukkan ke chain.
    // --------------------------------------------------------

    blocks.push_back(block);

    // --------------------------------------------------------
    // 6. Baru setelah block masuk chain, update UTXO.
    // --------------------------------------------------------

    utxoManager->ProcessBlock(block);

    return true;
}

void BerylChain::SetUTXOManager(
    UTXOManager* manager
)
{
    utxoManager = manager;
}

const UTXOManager* BerylChain::GetUTXOManager() const
{
    return utxoManager;
}

beryl::contract::ContractStateManager&
BerylChain::GetContractStateManager()
{
    return contractState;
}

const beryl::contract::ContractStateManager&
BerylChain::GetContractStateManager() const
{
    return contractState;
}


std::string BerylChain::GetLastHash() const
{

    if(blocks.empty())
        return "0";


    return blocks.back().hash;

}



int BerylChain::GetHeight() const
{

    return blocks.size();

}

const std::vector<BerylBlock>& BerylChain::GetBlocks() const
{

    return blocks;

}

const BerylBlock&
BerylChain::GetBlock(
    int index
) const
{
    return blocks.at(index);
}

const BerylBlock&
BerylChain::GetLastBlock() const
{
    return blocks.back();
}
