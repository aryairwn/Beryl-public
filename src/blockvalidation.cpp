// © Arya Irawan — 10 August 2026

#include "blockvalidation.h"

#include "blockhash.h"
#include "consensus.h"
#include "merkle.h"
#include "transaction.h"
#include "utxoroot.h"
#include "contract_state_manager.h"
#include "contract_engine.h"
#include "contract_context.h"
#include "contract_address.h"

#include <chrono>
#include <iostream>
#include <cstdint>
#include <limits>
#include <string>

// ============================================================
// CHECK PROOF OF WORK
// ============================================================
//
// Difficulty Beryl saat ini direpresentasikan sebagai jumlah
// karakter '0' hexadecimal yang harus berada di awal hash.
//
// Contoh:
// difficulty = 3
// hash harus diawali:
// 000...
//
// ============================================================

static bool CheckProofOfWork(
    const BerylBlock& block
)
{
    if (block.header.difficulty == 0)
    {
        std::cerr << "POW DEBUG: target=0\n";
        return false;
    }

    const std::string calculatedHash =
        GetBlockHash(block.header);

    if (calculatedHash != block.hash)
    {
        std::cerr
            << "POW DEBUG: HASH MISMATCH\n"
            << "  calculated=" << calculatedHash << "\n"
            << "  block.hash=" << block.hash << "\n";
        return false;
    }

    if (calculatedHash.size() < 16)
    {
        std::cerr << "POW DEBUG: hash too short\n";
        return false;
    }

    uint64_t hashValue = 0;

    for (int i = 0; i < 16; ++i)
    {
        const char c = calculatedHash[i];

        uint64_t nibble;

        if (c >= '0' && c <= '9')
            nibble = c - '0';
        else if (c >= 'a' && c <= 'f')
            nibble = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            nibble = c - 'A' + 10;
        else
        {
            std::cerr
                << "POW DEBUG: invalid hex character\n";
            return false;
        }

        hashValue =
            (hashValue << 4) | nibble;
    }

    std::cerr
        << "POW DEBUG: hash=" << calculatedHash
        << " hashValue=" << hashValue
        << " target=" << block.header.difficulty
        << " result="
        << (hashValue <= block.header.difficulty
                ? "PASS"
                : "FAIL")
        << "\n";

    if (hashValue > block.header.difficulty)
        return false;

    return true;
}

// ============================================================
// CONTRACT STATE TRANSITION
// ============================================================
//
// CONTRACT state adalah bagian consensus.
//
// Fungsi ini hanya memodifikasi ContractStateManager yang
// diberikan caller. ValidateBlock() memberikan COPY sehingga
// block invalid tidak pernah mengubah state asli.
//
// ============================================================

bool ApplyContractTransactionState(
    beryl::contract::ContractStateManager& contractState,
    const BerylTransaction& tx,
    int height
)
{
    (void)height;

    // Native transaction tidak menyentuh contract state.
    if (tx.type == TransactionType::NORMAL)
    {
        return true;
    }

    // tx.type dan contract.type wajib identik.
    if (tx.type != tx.contract.type)
    {
        return false;
    }

    // ========================================================
    // CONTRACT DEPLOY
    // ========================================================

    if (tx.type == TransactionType::CONTRACT_DEPLOY)
    {
        // Deployment V1 tidak melakukan VM execution.
        // Karena itu gas/input harus kosong.
        if (tx.contract.caller.empty())
            return false;

        if (tx.contract.bytecode.empty())
            return false;

        if (!tx.contract.input.empty())
            return false;

        if (tx.contract.gasLimit != 0)
            return false;

        // Contract address harus merupakan address yang
        // deterministically diturunkan dari caller + nonce.
        const std::string expectedAddress =
            beryl::contract::DeriveContractAddress(
                tx.contract.caller,
                tx.contract.deploymentNonce
            );

        if (expectedAddress.empty())
            return false;

        if (tx.contract.contractAddress != expectedAddress)
            return false;

        // Contract::Create() melakukan validasi bytecode
        // dan menghitung CodeHash secara deterministic.
        beryl::contract::Contract contract;

        std::string error;

        if (!contract.Create(
                tx.contract.bytecode,
                error
            ))
        {
            return false;
        }

        // Address tidak boleh pernah dipakai ulang.
        if (contractState.HasContract(
                expectedAddress
            ))
        {
            return false;
        }

        return contractState.Deploy(
            expectedAddress,
            contract
        );
    }

    // ========================================================
    // CONTRACT CALL
    // ========================================================

    if (tx.type == TransactionType::CONTRACT_CALL)
    {
        if (tx.contract.caller.empty())
            return false;

        if (tx.contract.contractAddress.empty())
            return false;

        // CALL tidak membawa bytecode/deployment nonce.
        if (!tx.contract.bytecode.empty())
            return false;

        if (tx.contract.deploymentNonce != 0)
            return false;

        if (tx.contract.gasLimit < beryl::bvm::MIN_GAS_LIMIT ||
            tx.contract.gasLimit > beryl::bvm::MAX_GAS_LIMIT)
        {
            return false;
        }

        // Address harus valid canonical Bech32 "bvm1...".
        beryl::contract::ContractId contractId{};

        if (!beryl::contract::DecodeContractAddress(
                tx.contract.contractAddress,
                contractId
            ))
        {
            return false;
        }

        const beryl::contract::Contract* contract =
            contractState.GetContract(
                tx.contract.contractAddress
            );

        if (contract == nullptr)
            return false;

        // ContractState mengikat storage kepada ContractId.
        beryl::contract::ContractState state(
            contractId,
            &contractState.Backend()
        );

        std::string error;

        beryl::contract::ExecutionContext context;

        if (!context.Initialize(
                tx.contract.caller,
                contractId,
                tx.contract.gasLimit,
                tx.contract.input,
                error
            ))
        {
            return false;
        }

        beryl::contract::ExecutionEngine engine;

        const beryl::bvm::Result result =
            engine.Execute(
                *contract,
                context,
                state
            );

        // Semua non-success execution adalah invalid block
        // untuk consensus V1.
        if (!result.Success())
            return false;

        return true;
    }

    // Unknown transaction type.
    return false;
}

// ============================================================
// VALIDATE BLOCK
// ============================================================

bool ValidateBlock(
    const BerylBlock& block,
    const BerylChain& chain,
    const UTXOManager& utxoManager
)
{
    auto RejectValidation = [](int line) -> bool
    {
        std::cerr
            << "VALIDATE REJECT LINE="
            << line
            << std::endl;
        return false;
    };

    // --------------------------------------------------------
    // 1. Block harus mempunyai transaksi.
    // --------------------------------------------------------

    if (block.transactions.empty())
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 1b. Ukuran block tidak boleh melebihi batas consensus.
    // --------------------------------------------------------
    const std::string serializedBlock =
        SerializeBerylBlock(block);

    if (serializedBlock.size() >
        BerylConsensus::MAX_BLOCK_SIZE)
    {
        return RejectValidation(__LINE__);
    }

    // --------------------------------------------------------
    // 2. Height harus sesuai posisi block berikutnya.
    // --------------------------------------------------------

    const int expectedHeight =
        chain.GetHeight() + 1;

    if (block.header.height != expectedHeight)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 3. Previous hash harus menunjuk block terakhir.
    // --------------------------------------------------------

    if (block.header.previousHash !=
        chain.GetLastHash())
    {
        return RejectValidation(__LINE__);
    }

    // --------------------------------------------------------
    // 4. Difficulty harus valid.
    // --------------------------------------------------------

    if (block.header.difficulty == 0)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 5. Merkle root harus cocok.
    // --------------------------------------------------------

    const std::string calculatedMerkle =
        CalculateMerkleRoot(block.transactions);

    if (calculatedMerkle != block.header.merkleRoot)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 6. UTXO root harus cocok dengan UTXO set setelah block
    //    diterapkan.
    //
    //    Gunakan copy agar UTXO asli tidak berubah selama
    //    proses validasi.
    // --------------------------------------------------------
    const auto utxoCopyStart =
        std::chrono::steady_clock::now();

    UTXOManager candidateUTXO = utxoManager;

    const auto utxoCopyEnd =
        std::chrono::steady_clock::now();

    const double utxoCopySeconds =
        std::chrono::duration<double>(
            utxoCopyEnd - utxoCopyStart
        ).count();

    std::cout
        << "VALIDATE: UTXO COPY TIME: "
        << utxoCopySeconds
        << " s\n";

    const auto candidateProcessStart =
        std::chrono::steady_clock::now();

    candidateUTXO.ProcessBlock(block);

    const auto candidateProcessEnd =
        std::chrono::steady_clock::now();

    const double candidateProcessSeconds =
        std::chrono::duration<double>(
            candidateProcessEnd - candidateProcessStart
        ).count();

    std::cout
        << "VALIDATE: UTXO PROCESS TIME: "
        << candidateProcessSeconds
        << " s\n";

    const auto utxoRootStart =
        std::chrono::steady_clock::now();

    const std::string calculatedUTXORoot =
        CalculateUTXORoot(candidateUTXO);

    const auto utxoRootEnd =
        std::chrono::steady_clock::now();

    const double utxoRootSeconds =
        std::chrono::duration<double>(
            utxoRootEnd - utxoRootStart
        ).count();

    std::cout
        << "VALIDATE: UTXO ROOT TIME: "
        << utxoRootSeconds
        << " s\n";

    if (calculatedUTXORoot.empty())
        return RejectValidation(__LINE__);

    if (calculatedUTXORoot != block.header.utxoRoot)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 7. Proof-of-Work harus valid.
    // --------------------------------------------------------

    if (!CheckProofOfWork(block))
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 8. Coinbase harus berada di posisi pertama.
    // --------------------------------------------------------

    const BerylTransaction& coinbase =
        block.transactions.front();

    if (!coinbase.vin.empty())
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 9. Hanya boleh ada satu coinbase.
    // --------------------------------------------------------

    for (size_t i = 1;
         i < block.transactions.size();
         ++i)
    {
        if (block.transactions[i].vin.empty())
            return RejectValidation(__LINE__);
    }

    // --------------------------------------------------------
    // 10. Coinbase harus mempunyai output.
    // --------------------------------------------------------

    if (coinbase.vout.empty())
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 11. Coinbase output harus valid.
    // --------------------------------------------------------

    uint64_t coinbaseAmount = 0;

    for (const auto& out : coinbase.vout)
    {
        if (out.amount == 0)
            return RejectValidation(__LINE__);

        if (UINT64_MAX - coinbaseAmount < out.amount)
            return RejectValidation(__LINE__);

        coinbaseAmount += out.amount;
    }

    // --------------------------------------------------------
    // 11b. Metadata block.reward harus sama dengan
    //      jumlah seluruh output coinbase.
    // --------------------------------------------------------

    if (block.reward != coinbaseAmount)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 12. Validasi seluruh transaksi non-coinbase.
    // --------------------------------------------------------

    // ========================================================
    // 12. VALIDASI TRANSAKSI SECARA SEQUENTIAL
    // ========================================================
    // Setiap transaksi berikutnya harus melihat perubahan UTXO
    // yang dibuat oleh transaksi sebelumnya dalam block yang sama.
    //
    // Contoh:
    //
    // TX1: A -> B
    // TX2: B -> C
    //
    // TX2 sekarang dapat menggunakan output TX1 dalam block
    // yang sama.
    //
    // Candidate UTXO hanya dipakai selama validasi dan tidak
    // mengubah UTXOManager asli.
    // ========================================================

    UTXOManager validationUTXO = utxoManager;

    // Candidate contract state.
    // Validasi block tidak boleh mengubah persistent contract state.
    // Semua contract transition selama validasi diterapkan
    // pada copy ini.
    beryl::contract::ContractStateManager validationContractState =
        chain.GetContractStateManager();

    uint64_t totalFees = 0;

    for (size_t i = 1;
         i < block.transactions.size();
         ++i)
    {
        const BerylTransaction& tx =
            block.transactions[i];

        // Validasi terhadap state UTXO TERKINI.
        if (!ValidateTransaction(
                validationUTXO.GetUTXOSet(),
                tx,
                block.header.height
            ))
        {
            return RejectValidation(__LINE__);
        }

        const uint64_t fee =
            CalculateTransactionFee(
                validationUTXO.GetUTXOSet(),
                tx
            );

        if (fee < MIN_TRANSACTION_FEE)
            return RejectValidation(__LINE__);

        if (UINT64_MAX - totalFees < fee)
            return RejectValidation(__LINE__);

        totalFees += fee;

        // Terapkan TX ke candidate state.
        // TX berikutnya akan melihat hasil TX ini.
        if (!ApplyTransaction(
                validationUTXO.GetUTXOSetMutable(),
                tx,
                block.header.height
            ))
        {
            return RejectValidation(__LINE__);
        }

        // Apply contract state after the transaction's
        // UTXO transition has been validated/applied.
        if (!ApplyContractTransactionState(
                validationContractState,
                tx,
                block.header.height
            ))
        {
            return RejectValidation(__LINE__);
        }
    }

    // --------------------------------------------------------
    // 13. Contract state root.
    //
    // Root harus merepresentasikan contract registry +
    // persistent contract storage setelah seluruh TX block
    // diterapkan secara sequential.
    // --------------------------------------------------------

    const std::string calculatedContractRoot =
        validationContractState.CalculateRoot();

    if (calculatedContractRoot.empty())
        return RejectValidation(__LINE__);

    if (calculatedContractRoot != block.header.contractRoot)
    {
        std::cerr
            << "CONTRACT ROOT MISMATCH"
            << " height=" << block.header.height
            << "\n  calculated="
            << calculatedContractRoot
            << "\n  header="
            << block.header.contractRoot
            << "\n";

        return RejectValidation(__LINE__);
    }

    // --------------------------------------------------------
    // 14. Hitung subsidy berdasarkan height.
    // --------------------------------------------------------

    const uint64_t subsidy =
        BerylConsensus::GetBlockSubsidy(
            block.header.height
        );

    // --------------------------------------------------------
    // 15. Coinbase maksimal = subsidy + seluruh fee.
    // --------------------------------------------------------

    if (UINT64_MAX - subsidy < totalFees)
        return RejectValidation(__LINE__);

    const uint64_t maximumCoinbase =
        subsidy + totalFees;

    if (coinbaseAmount > maximumCoinbase)
        return RejectValidation(__LINE__);

    // --------------------------------------------------------
    // 16. Block valid.
    // --------------------------------------------------------

    return true;
}
