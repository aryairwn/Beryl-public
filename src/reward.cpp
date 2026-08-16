// © Arya Irawan — 10 August 2026

#include "reward.h"
#include <cstdint>
#include "consensus.h"

// Legacy compatibility wrapper.
//
// Semua aturan subsidy/reward consensus Beryl harus berasal dari:
// BerylConsensus::GetBlockSubsidy(height)
//
// Wrapper ini dipertahankan agar kode lama yang masih memanggil
// GetBlockReward() tetap konsisten dengan consensus terbaru,
// termasuk aturan hard cap MAX_SUPPLY.

uint64_t GetBlockReward(uint64_t height)
{
    if (height > static_cast<uint64_t>(INT32_MAX))
        return 0;

    return BerylConsensus::GetBlockSubsidy(
        static_cast<int>(height)
    );
}
