// © Arya Irawan — 10 August 2026

#include "validation.h"
#include "blockhash.h"
#include "consensus.h"

#include <iostream>

bool CheckBlock(
    const BerylBlock& block,
    const std::string& previousHash
)
{

    // Cek previous hash
    if(block.header.previousHash != previousHash)
    {
        return false;
    }

    // Hitung ulang hash
    std::string calculated =
        GetBlockHash(block.header);

    if(calculated != block.hash)
    {
        return false;
    }

    // ========================================================
    // PROOF OF WORK BERYL
    //
    // PoW tunggal untuk chain baru:
    //
    //   hash64 <= target
    //
    // Target adalah uint64_t.
    // Target lebih besar = mining lebih mudah.
    // Target lebih kecil = mining lebih sulit.
    // ========================================================

    bool validPoW = true;

    if (block.header.difficulty == 0)
    {
        validPoW = false;
    }
    else if (block.hash.size() < 16)
    {
        validPoW = false;
    }
    else
    {
        uint64_t hashValue = 0;

        for (int i = 0; i < 16; ++i)
        {
            const char c = block.hash[i];

            uint64_t nibble;

            if (c >= '0' && c <= '9')
            {
                nibble =
                    static_cast<uint64_t>(c - '0');
            }
            else if (c >= 'a' && c <= 'f')
            {
                nibble =
                    static_cast<uint64_t>(c - 'a' + 10);
            }
            else if (c >= 'A' && c <= 'F')
            {
                nibble =
                    static_cast<uint64_t>(c - 'A' + 10);
            }
            else
            {
                validPoW = false;
                break;
            }

            hashValue =
                (hashValue << 4) | nibble;
        }

        if (validPoW &&
            hashValue > block.header.difficulty)
        {
            validPoW = false;
        }
    }

    if (!validPoW)
    {
        std::cout
            << "INVALID: bad proof of work\n";

        return false;
    }

    // Cek reward sesuai aturan consensus Beryl
    const uint64_t expectedReward =
        BerylConsensus::GetBlockSubsidy(
            block.header.height
        );

    if(block.reward != expectedReward)
    {
        return false;
    }

    return true;
}
