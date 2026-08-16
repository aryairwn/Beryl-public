// © Arya Irawan — 10 August 2026

#include "genesis.h"
#include "blockhash.h"
#include "consensus.h"
#include "coinbase.h"
#include "merkle.h"
#include "utxoroot.h"
#include "utxomanager.h"
#include "contract_state_manager.h"

#include <cstdint>
#include <string>

/*
 * ============================================================
 * BERYL REAL GENESIS BLOCK
 * ============================================================
 *
 * Genesis harus deterministik.
 *
 * Jangan menggunakan time(nullptr).
 * Semua node Beryl harus menghasilkan genesis yang sama.
 *
 * Genesis:
 *   height       = 1
 *   previousHash = "0"
 *   difficulty   = INITIAL_DIFFICULTY
 *   reward       = INITIAL_REWARD
 *
 * Nonce dicari menggunakan GetBlockHash(), sehingga PoW
 * menggunakan YesPower yang sama dengan blockchain Beryl.
 * ============================================================
 */

BerylBlock CreateGenesisBlock()
{
    BerylBlock genesis;

    // --------------------------------------------------------
    // Genesis identity
    // --------------------------------------------------------

    genesis.header.version = 1;
    genesis.header.height = 1;
    genesis.header.previousHash = "0";

    // Timestamp tetap agar genesis identik di semua node.
    genesis.header.timestamp = 1754600000ULL;

    // Beryl PoW menggunakan target numerik 64-bit.
    //
    // hash64 <= target
    //
    // Target awal dirancang untuk sekitar 600 expected hashes.
    genesis.header.difficulty =
        BerylConsensus::INITIAL_POW_TARGET;

    genesis.reward =
        BerylConsensus::INITIAL_REWARD;

    // --------------------------------------------------------
    // Genesis coinbase
    // --------------------------------------------------------

    const std::string genesisAddress =
        "berGENESIS";

    BerylTransaction coinbase =
        CreateCoinbaseTransaction(
            genesisAddress,
            BerylConsensus::INITIAL_REWARD
        );

    genesis.transactions.clear();

    genesis.transactions.push_back(
        coinbase
    );

    // --------------------------------------------------------
    // Merkle Root
    // --------------------------------------------------------

    genesis.header.merkleRoot =
        CalculateMerkleRoot(
            genesis.transactions
        );

    // --------------------------------------------------------
    // Genesis UTXO commitment
    // --------------------------------------------------------

    UTXOManager genesisUTXO;

    genesisUTXO.ProcessBlock(
        genesis
    );

    genesis.header.utxoRoot =
        CalculateUTXORoot(
            genesisUTXO
        );

    if (genesis.header.utxoRoot.empty())
    {
        return genesis;
    }

    // --------------------------------------------------------
    // Genesis Contract State commitment
    // --------------------------------------------------------

    beryl::contract::ContractStateManager genesisContractState;

    genesis.header.contractRoot =
        genesisContractState.CalculateRoot();

    if (genesis.header.contractRoot.empty())
    {
        return genesis;
    }

    // --------------------------------------------------------
    // Genesis PoW
    //
    // Aturan SAMA dengan miner dan validator:
    //
    //     hash64 <= difficulty
    //
    // Tidak menggunakan leading-zero.
    // --------------------------------------------------------

    const uint64_t target =
        BerylConsensus::INITIAL_POW_TARGET;

    for (uint32_t nonce = 0;; ++nonce)
    {
        genesis.header.nonce = nonce;

        genesis.hash =
            GetBlockHash(
                genesis.header
            );

        if (genesis.hash.size() < 16)
            continue;

        uint64_t hashValue = 0;
        bool validHex = true;

        for (int i = 0; i < 16; ++i)
        {
            const char c =
                genesis.hash[i];

            uint64_t nibble = 0;

            if (c >= '0' && c <= '9')
            {
                nibble =
                    static_cast<uint64_t>(
                        c - '0'
                    );
            }
            else if (c >= 'a' && c <= 'f')
            {
                nibble =
                    static_cast<uint64_t>(
                        c - 'a' + 10
                    );
            }
            else if (c >= 'A' && c <= 'F')
            {
                nibble =
                    static_cast<uint64_t>(
                        c - 'A' + 10
                    );
            }
            else
            {
                validHex = false;
                break;
            }

            hashValue =
                (hashValue << 4) | nibble;
        }

        if (validHex && hashValue <= target)
        {
            break;
        }
    }

    return genesis;
}
