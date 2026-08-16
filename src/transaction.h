// © Arya Irawan — 10 August 2026

#ifndef BERYL_TRANSACTION_H
#define BERYL_TRANSACTION_H

#include <string>
#include <vector>
#include <cstdint>
#include <climits>

#include "crypto/falcon/falcon.h"
#include "utxo.h"

// ============================================================
// TRANSACTION FEE
// ============================================================

// 1 unit terkecil = 0.00000001 BER
static constexpr uint64_t MIN_TRANSACTION_FEE = 1;


// ============================================================
// TRANSACTION TYPE
// ============================================================
//
// NORMAL
// CONTRACT_DEPLOY
// CONTRACT_CALL
//
// Contract menjadi bagian native dari BerylTransaction.
// ============================================================

enum class TransactionType : uint8_t
{
    NORMAL = 0,
    CONTRACT_DEPLOY = 1,
    CONTRACT_CALL = 2
};

// ============================================================
// CONTRACT TRANSACTION PAYLOAD
// ============================================================

struct ContractTransaction
{
    TransactionType type = TransactionType::NORMAL;

    // Caller/deployer account identifier.
    // Tidak menyimpan private key atau seed.
    std::vector<unsigned char> caller;

    // Digunakan untuk CONTRACT_CALL.
    // Format: bvm1...
    std::string contractAddress;

    // Digunakan untuk deterministic contract deployment.
    uint64_t deploymentNonce = 0;

    // Digunakan hanya CONTRACT_DEPLOY.
    std::vector<unsigned char> bytecode;

    // Digunakan hanya CONTRACT_CALL.
    std::vector<unsigned char> input;

    // Maximum gas yang bersedia digunakan execution.
    uint64_t gasLimit = 0;
};

// ============================================================
// TX INPUT
// ============================================================

struct TxInput
{
    std::string previousTx;
    uint32_t outputIndex;

    std::string publicKey;
    std::string signature;
};

// ============================================================
// TX OUTPUT
// ============================================================

struct TxOutput
{
    std::string address;
    uint64_t amount;

    bool spent = false;
};

// ============================================================
// TRANSACTION
// ============================================================

struct BerylTransaction
{
    std::string txid;

    // Native Beryl transaction type.
    TransactionType type = TransactionType::NORMAL;

    // Contract-specific payload.
    ContractTransaction contract;

    // Data khusus coinbase.
    // Transaksi biasa dibiarkan kosong.
    // Data ini ikut dihitung ke TXID.
    std::string coinbaseData;

    std::vector<TxInput> vin;
    std::vector<TxOutput> vout;

    // Transaction-level Falcon key/signature
    std::vector<unsigned char> signature;
    std::vector<unsigned char> publicKey;
};

// ============================================================
// TX ID
// ============================================================

std::string CalculateTxID(
    const BerylTransaction& tx
);

std::string CalculateSigningHash(
    const BerylTransaction& tx
);

// ============================================================
// PUBLIC TRANSACTION SERIALIZATION
// Digunakan oleh Beryl Wallet <-> Beryl Node.
// Format harus kompatibel dengan wire transaction P2P.
// ============================================================

std::string SerializeBerylTransaction(
    const BerylTransaction& tx
);

bool DeserializeBerylTransaction(
    const std::string& data,
    BerylTransaction& tx
);

// ============================================================
// SIMPLE UTXO FUNCTIONS
// ============================================================

void AddUTXO(
    const std::string& txid,
    uint32_t index,
    const TxOutput& out
);

bool SpendUTXO(
    const std::string& txid,
    uint32_t index
);

bool GetUTXO(
    const std::string& txid,
    uint32_t index,
    TxOutput& out
);

// ============================================================
// TRANSACTION CREATION
// ============================================================

bool CreateTransaction(
    UTXOSet& utxos,
    const std::string& from,
    const std::string& to,
    uint64_t amount,
    BerylTransaction& tx,
    int currentHeight = -1,
    uint64_t fee = MIN_TRANSACTION_FEE,
    const std::vector<std::string>& reservedInputs = {}
);

// ============================================================
// FALCON TRANSACTION SIGNATURE
// ============================================================

bool SignTransaction(
    BerylTransaction& tx,
    FalconKey& key
);

bool VerifyTransactionSignature(
    const BerylTransaction& tx
);

// ============================================================
// TRANSACTION VALIDATION
// ============================================================

bool ValidateTransaction(
    const UTXOSet& utxos,
    const BerylTransaction& tx,
    int currentHeight = INT_MAX
);

// ============================================================
// TRANSACTION FEE CALCULATION
// ============================================================

uint64_t CalculateTransactionFee(
    const UTXOSet& utxos,
    const BerylTransaction& tx
);

// ============================================================
// BLOCK TRANSACTION FEES
// ============================================================

// Menghitung total fee seluruh transaksi non-coinbase
// dalam satu block.
uint64_t CalculateBlockFees(
    const UTXOSet& utxos,
    const std::vector<BerylTransaction>& transactions
);

// ============================================================
// APPLY TRANSACTION
// ============================================================

bool ApplyTransaction(
    UTXOSet& utxos,
    const BerylTransaction& tx,
    int currentHeight = -1
);

#endif
