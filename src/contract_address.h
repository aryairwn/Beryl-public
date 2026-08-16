#ifndef BERYL_CONTRACT_ADDRESS_H
#define BERYL_CONTRACT_ADDRESS_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT ADDRESS V1
//
// Account address:
//     ber1...
//
// Contract address:
//     bvm1...
//
// Contract identity:
//     BLAKE3-256(
//         "BVM1" ||
//         deployer_account_id ||
//         deployment_nonce
//     )
//
// Belum menjadi consensus.
// ============================================================

static constexpr size_t CONTRACT_ID_SIZE = 32;

using ContractId = std::array<uint8_t, CONTRACT_ID_SIZE>;

// Deterministically derive contract ID.
ContractId DeriveContractId(
    const std::vector<uint8_t>& deployerAccountId,
    uint64_t deploymentNonce
);

// Encode contract ID as Bech32 with HRP "bvm".
std::string EncodeContractAddress(
    const ContractId& contractId
);

// Decode canonical Bech32 contract address (HRP: bvm)
// menjadi ContractId 32-byte.
bool DecodeContractAddress(
    const std::string& address,
    ContractId& contractId
);

// Derive and encode directly.
std::string DeriveContractAddress(
    const std::vector<uint8_t>& deployerAccountId,
    uint64_t deploymentNonce
);

} // namespace beryl::contract

#endif
