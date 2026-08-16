// © Arya Irawan — 10 August 2026

#include "difficulty.h"
#include "consensus.h"

#include <algorithm>
#include <cstdint>
#include <limits>

uint64_t CalculateDifficulty(
    const BerylChain& chain
)
{
    const int height = chain.GetHeight();

    // Genesis / chain kosong.
    if (height == 0)
        return BerylConsensus::INITIAL_POW_TARGET;

    uint64_t target =
        chain.GetLastBlock().header.difficulty;

    if (target == 0)
        target = BerylConsensus::INITIAL_POW_TARGET;

    // --------------------------------------------------------
    // Retarget setiap 100 blok.
    //
    // Target block time:
    //
    //     100 × 6 = 600 detik
    //
    // Jika block terlalu cepat:
    //     actual < target
    //     -> target turun
    //     -> mining lebih sulit.
    //
    // Jika block terlalu lambat:
    //     actual > target
    //     -> target naik
    //     -> mining lebih mudah.
    // --------------------------------------------------------

    if (
        height >=
        static_cast<int>(
            BerylConsensus::DIFFICULTY_ADJUSTMENT_INTERVAL
        )
        &&
        height %
            static_cast<int>(
                BerylConsensus::DIFFICULTY_ADJUSTMENT_INTERVAL
            ) == 0
    )
    {
        const int last =
            height - 1;

        // --------------------------------------------------------
        // Difficulty retarget Beryl
        //
        // Retarget dilakukan setiap 100 blok (~10 menit).
        // Namun waktu aktual diukur dari 100 blok terakhir
        // agar difficulty lebih responsif terhadap perubahan
        // total hash rate jaringan.
        //
        // Genesis tidak pernah digunakan sebagai titik awal
        // pengukuran waktu.
        // --------------------------------------------------------

        constexpr int MEASUREMENT_BLOCKS = 100;

        int first =
            last - MEASUREMENT_BLOCKS;

        // Timestamp Genesis tidak digunakan sebagai
        // titik awal pengukuran retarget.
        if (first < 1)
            first = 1;

        if (first >= last)
            return target;

        const BerylBlock& firstBlock =
            chain.GetBlock(first);

        const BerylBlock& lastBlock =
            chain.GetBlock(last);

        uint64_t actualTime =
            lastBlock.header.timestamp -
            firstBlock.header.timestamp;

        if (actualTime == 0)
            actualTime = 1;

        // 100 interval × 6 detik = 600 detik.
        const uint64_t targetTime =
            static_cast<uint64_t>(
                MEASUREMENT_BLOCKS
            ) *
            BerylConsensus::BLOCK_TIME;

        // target_new = target_old × actual / target
        //
        // actual lebih kecil -> target lebih kecil
        // -> mining lebih sulit.
        //
        // actual lebih besar -> target lebih besar
        // -> mining lebih mudah.
        __uint128_t adjusted =
            static_cast<__uint128_t>(target) *
            actualTime /
            targetTime;

        // Batasi perubahan maksimal 4x per retarget.
        const uint64_t minTarget =
            std::max<uint64_t>(
                1,
                target / 4
            );

        const uint64_t maxTarget =
            target >
                (UINT64_MAX / 4)
                ? UINT64_MAX
                : target * 4;

        uint64_t newTarget;

        if (adjusted > UINT64_MAX)
            newTarget = UINT64_MAX;
        else
            newTarget =
                static_cast<uint64_t>(adjusted);

        newTarget =
            std::clamp(
                newTarget,
                minTarget,
                maxTarget
            );

        target = newTarget;
    }

    if (target == 0)
        target = 1;

    return target;
}
