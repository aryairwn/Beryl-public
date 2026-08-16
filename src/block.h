// © Arya Irawan — 10 August 2026

#ifndef BERYL_BLOCK_H
#define BERYL_BLOCK_H

#include <string>
#include <cstdint>
#include <vector>
#include "transaction.h"

struct BerylHeader
{
    int version;

    std::string previousHash;

    std::string merkleRoot;

    // Deterministic commitment terhadap UTXO set
    // setelah block ini diterapkan.
    std::string utxoRoot;

    // Deterministic commitment terhadap Contract State
    // setelah block ini diterapkan.
    std::string contractRoot;

    uint64_t timestamp;

    uint32_t nonce;

    uint64_t difficulty;

    int height;
};

struct BerylBlock
{
    BerylHeader header;

    // sementara reward masih dipakai
    uint64_t reward;

    // transaksi dalam block
    std::vector<BerylTransaction> transactions;

    std::string hash;
};

// ============================================================
// CANONICAL BLOCK SERIALIZATION
// ============================================================
// Format ini dipakai bersama oleh:
//   - miner
//   - block validation
//   - P2P
//
// Tujuannya agar ukuran block yang dihitung consensus
// identik dengan ukuran block yang dikirim melalui jaringan.
// ============================================================

std::string SerializeBerylBlock(
    const BerylBlock& block
);

// ============================================================
// CANONICAL BLOCK DESERIALIZATION
// ============================================================
// Dipakai oleh RPC:
//   getblocktemplate -> blockhex
//   submitblock     -> blockhex
// ============================================================

bool DeserializeBerylBlock(
    const std::string& data,
    BerylBlock& block
);


#endif
