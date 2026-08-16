// © Arya Irawan — 10 August 2026

#include "consensus.h"

namespace BerylConsensus
{

uint64_t GetBlockSubsidy(int height)
{
    if (height < 0)
        return 0;

    // ========================================================
    // 1. Reward normal berdasarkan tahun
    // ========================================================

    uint64_t reward = INITIAL_REWARD;

    const uint64_t year =
        static_cast<uint64_t>(height) / BLOCKS_PER_YEAR;

    // Reward turun 10% setiap tahun secara compound.
    for (uint64_t i = 0; i < year; ++i)
    {
        reward =
            (reward * REWARD_NUMERATOR) /
            REWARD_DENOMINATOR;

        if (reward == 0)
            return 0;
    }

    // ========================================================
    // 2. Hitung total subsidy yang sudah diterbitkan
    //    SEBELUM block "height".
    //
    //    Perhitungan dilakukan per tahun agar efisien.
    // ========================================================

    uint64_t issued = 0;

    // Semua tahun penuh sebelum tahun berjalan.
    uint64_t yearReward = INITIAL_REWARD;

    for (uint64_t y = 0; y < year; ++y)
    {
        if (yearReward != 0)
        {
            const uint64_t yearlyIssued =
                yearReward * BLOCKS_PER_YEAR;

            if (issued >= MAX_SUPPLY ||
                MAX_SUPPLY - issued < yearlyIssued)
            {
                return 0;
            }

            issued += yearlyIssued;
        }

        yearReward =
            (yearReward * REWARD_NUMERATOR) /
            REWARD_DENOMINATOR;

        if (yearReward == 0)
            break;
    }

    // Blok-blok yang sudah diterbitkan pada tahun berjalan.
    const uint64_t blocksIntoYear =
        static_cast<uint64_t>(height) % BLOCKS_PER_YEAR;

    if (blocksIntoYear != 0 && reward != 0)
    {
        const uint64_t currentYearIssued =
            reward * blocksIntoYear;

        if (issued >= MAX_SUPPLY ||
            MAX_SUPPLY - issued < currentYearIssued)
        {
            return 0;
        }

        issued += currentYearIssued;
    }

    // ========================================================
    // 3. Hard cap
    //
    //    Reward normal tidak boleh membuat total supply
    //    melebihi MAX_SUPPLY.
    // ========================================================

    if (issued >= MAX_SUPPLY)
        return 0;

    const uint64_t remaining =
        MAX_SUPPLY - issued;

    if (reward > remaining)
        return remaining;

    return reward;
}

}
