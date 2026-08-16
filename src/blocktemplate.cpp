// © Arya Irawan — 11 August 2026

#include "blocktemplate.h"

#include "blockvalidation.h"
#include "coinbase.h"
#include "consensus.h"
#include "difficulty.h"
#include "merkle.h"
#include "transaction.h"
#include "utxomanager.h"
#include "utxoroot.h"
#include "contract_state_manager.h"

#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <string>

bool CreateBlockTemplate(
    BerylBlock& block,
    BerylChain& chain,
    Mempool& mempool,
    const std::string& minerAddress
)
{
    // --------------------------------------------------------
    // 1. UTXO manager wajib tersedia.
    // --------------------------------------------------------

    const UTXOManager* utxoManager =
        chain.GetUTXOManager();

    if (utxoManager == nullptr)
        return false;

    // --------------------------------------------------------
    // 2. Tentukan height block berikutnya.
    // --------------------------------------------------------

    const int height =
        chain.GetHeight() + 1;

    block.header.version = 1;

    block.header.height =
        height;

    block.header.previousHash =
        chain.GetLastHash();

    block.header.timestamp =
        static_cast<uint64_t>(time(nullptr));

    block.header.difficulty =
        CalculateDifficulty(chain);

    block.header.nonce = 0;

    // --------------------------------------------------------
    // 3. Subsidy berdasarkan height.
    // --------------------------------------------------------

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(height);

    // --------------------------------------------------------
    // 4. Ambil transaksi dari mempool.
    // --------------------------------------------------------

    block.transactions.clear();

    const auto& mempoolTransactions =
        mempool.GetTransactions();

    // --------------------------------------------------------
    // 4b. Pilih transaksi secara sequential.
    //
    // Candidate UTXO memastikan transaksi berikutnya dapat
    // melihat perubahan UTXO dari transaksi sebelumnya.
    // --------------------------------------------------------

    UTXOManager selectionUTXO =
        *utxoManager;

    uint64_t totalFees = 0;

    for (const auto& tx : mempoolTransactions)
    {
        if (!ValidateTransaction(
                selectionUTXO.GetUTXOSet(),
                tx,
                height
            ))
        {
            continue;
        }

        const uint64_t fee =
            CalculateTransactionFee(
                selectionUTXO.GetUTXOSet(),
                tx
            );

        if (fee < MIN_TRANSACTION_FEE)
            continue;

        if (UINT64_MAX - totalFees < fee)
            continue;

        const uint64_t candidateFees =
            totalFees + fee;

        if (UINT64_MAX - subsidy < candidateFees)
            continue;

        const uint64_t candidateReward =
            subsidy + candidateFees;

        BerylTransaction candidateCoinbase =
            CreateCoinbaseTransaction(
                minerAddress,
                candidateReward,
                "BERYL-COINBASE-HEIGHT-" +
                std::to_string(height)
            );

        BerylBlock candidateBlock =
            block;

        candidateBlock.transactions.clear();

        candidateBlock.transactions.push_back(
            candidateCoinbase
        );

        for (const auto& selectedTx :
             block.transactions)
        {
            candidateBlock.transactions.push_back(
                selectedTx
            );
        }

        candidateBlock.transactions.push_back(tx);

        // Root sementara dibuat canonical 64 hex.
        candidateBlock.header.merkleRoot =
            std::string(64, '0');

        candidateBlock.header.utxoRoot =
            std::string(64, '0');

        candidateBlock.hash =
            std::string(64, '0');

        const std::string serializedCandidate =
            SerializeBerylBlock(candidateBlock);

        if (serializedCandidate.size() >
            BerylConsensus::MAX_BLOCK_SIZE)
        {
            continue;
        }

        if (!ApplyTransaction(
                selectionUTXO.GetUTXOSetMutable(),
                tx,
                height
            ))
        {
            continue;
        }

        block.transactions.push_back(tx);
        totalFees = candidateFees;
    }

    // --------------------------------------------------------
    // 5. Coinbase reward = subsidy + fee.
    // --------------------------------------------------------

    if (UINT64_MAX - subsidy < totalFees)
        return false;

    const uint64_t coinbaseReward =
        subsidy + totalFees;

    // --------------------------------------------------------
    // 6. Coinbase harus menjadi transaksi pertama.
    // --------------------------------------------------------

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            minerAddress,
            coinbaseReward,
            "BERYL-COINBASE-HEIGHT-" +
            std::to_string(height)
        );

    block.transactions.insert(
        block.transactions.begin(),
        coinbase
    );

    block.reward =
        coinbaseReward;

    // --------------------------------------------------------
    // 7. Merkle root final.
    // --------------------------------------------------------

    block.header.merkleRoot =
        CalculateMerkleRoot(
            block.transactions
        );

    // --------------------------------------------------------
    // 8. UTXO root final.
    // --------------------------------------------------------

    UTXOManager candidateUTXO =
        *utxoManager;

    candidateUTXO.ProcessBlock(block);

    block.header.utxoRoot =
        CalculateUTXORoot(candidateUTXO);

    if (block.header.utxoRoot.empty())
    {
        std::cerr
            << "NODE ERROR: gagal menghitung UTXO root\n";

        return false;
    }

    // --------------------------------------------------------
    // 8b. CONTRACT STATE ROOT FINAL.
    //
    // Harus menggunakan ContractState yang sama dengan
    // ValidateBlock(): state chain saat ini + seluruh TX
    // non-coinbase dalam block secara sequential.
    // --------------------------------------------------------

    beryl::contract::ContractStateManager
        selectionContractState =
            chain.GetContractStateManager();

    for (size_t i = 1;
         i < block.transactions.size();
         ++i)
    {
        if (!ApplyContractTransactionState(
                selectionContractState,
                block.transactions[i],
                height
            ))
        {
            std::cerr
                << "NODE ERROR: gagal menerapkan "
                << "contract transaction ke template\n";

            return false;
        }
    }

    block.header.contractRoot =
        selectionContractState.CalculateRoot();

    if (block.header.contractRoot.empty())
    {
        std::cerr
            << "NODE ERROR: gagal menghitung CONTRACT root\n";

        return false;
    }

    std::cerr
        << "TEMPLATE CONTRACT ROOT="
        << block.header.contractRoot
        << "\n";

    // --------------------------------------------------------
    // 9. Validasi ukuran block final.
    // --------------------------------------------------------

    const std::string serializedFinalBlock =
        SerializeBerylBlock(block);

    if (serializedFinalBlock.size() >
        BerylConsensus::MAX_BLOCK_SIZE)
    {
        std::cerr
            << "NODE ERROR: final block melebihi "
            << "MAX_BLOCK_SIZE\n";

        return false;
    }

    // --------------------------------------------------------
    // Block template siap.
    //
    // Node TIDAK melakukan Proof of Work.
    // Miner eksternal yang mengerjakan nonce.
    // --------------------------------------------------------

    return true;
}
