#include "consensus.h"

#include <iostream>
#include <cstdint>

static void PrintReward(
    const char* label,
    uint64_t reward
)
{
    uint64_t whole =
        reward / BerylConsensus::COIN;

    uint64_t fraction =
        reward % BerylConsensus::COIN;

    std::cout
        << label
        << " = "
        << reward
        << " units ("
        << whole
        << ".";

    // Selalu tampilkan 8 digit unit pecahan.
    if (fraction < 10000000ULL)
        std::cout << "0";
    if (fraction < 1000000ULL)
        std::cout << "0";
    if (fraction < 100000ULL)
        std::cout << "0";
    if (fraction < 10000ULL)
        std::cout << "0";
    if (fraction < 1000ULL)
        std::cout << "0";
    if (fraction < 100ULL)
        std::cout << "0";
    if (fraction < 10ULL)
        std::cout << "0";

    std::cout
        << fraction
        << " BER)"
        << "\n";
}

int main()
{
    using namespace BerylConsensus;

    std::cout << "=== BERYL CONSENSUS REWARD TEST ===\n";

    // --------------------------------------------------------
    // Tahun 0
    // --------------------------------------------------------

    uint64_t r0 =
        GetBlockSubsidy(0);

    if (r0 != INITIAL_REWARD)
    {
        std::cout << "YEAR 0 REWARD FAIL\n";
        return 1;
    }

    PrintReward("YEAR 0", r0);
    std::cout << "YEAR 0 REWARD OK\n";

    // --------------------------------------------------------
    // Blok terakhir tahun 0
    // --------------------------------------------------------

    uint64_t r0_last =
        GetBlockSubsidy(
            BLOCKS_PER_YEAR - 1
        );

    if (r0_last != INITIAL_REWARD)
    {
        std::cout << "YEAR 0 BOUNDARY FAIL\n";
        return 1;
    }

    std::cout
        << "YEAR 0 BOUNDARY OK\n";

    // --------------------------------------------------------
    // Awal tahun 1 = 36 BER
    // --------------------------------------------------------

    uint64_t r1 =
        GetBlockSubsidy(
            BLOCKS_PER_YEAR
        );

    uint64_t expected1 =
        INITIAL_REWARD * 9 / 10;

    if (r1 != expected1)
    {
        std::cout << "YEAR 1 REWARD FAIL\n";
        return 1;
    }

    PrintReward("YEAR 1", r1);
    std::cout << "YEAR 1 REWARD OK\n";

    // --------------------------------------------------------
    // Awal tahun 2 = 32,4 BER
    // --------------------------------------------------------

    uint64_t r2 =
        GetBlockSubsidy(
            BLOCKS_PER_YEAR * 2
        );

    uint64_t expected2 =
        expected1 * 9 / 10;

    if (r2 != expected2)
    {
        std::cout << "YEAR 2 REWARD FAIL\n";
        return 1;
    }

    PrintReward("YEAR 2", r2);
    std::cout << "YEAR 2 REWARD OK\n";

    // --------------------------------------------------------
    // Awal tahun 3 = 29,16 BER
    // --------------------------------------------------------

    uint64_t r3 =
        GetBlockSubsidy(
            BLOCKS_PER_YEAR * 3
        );

    uint64_t expected3 =
        expected2 * 9 / 10;

    if (r3 != expected3)
    {
        std::cout << "YEAR 3 REWARD FAIL\n";
        return 1;
    }

    PrintReward("YEAR 3", r3);
    std::cout << "YEAR 3 REWARD OK\n";

    // --------------------------------------------------------
    // Reward harus selalu turun / tidak naik
    // --------------------------------------------------------

    if (!(r3 < r2 &&
          r2 < r1 &&
          r1 < r0))
    {
        std::cout
            << "REWARD MONOTONICITY FAIL\n";
        return 1;
    }

    std::cout
        << "REWARD MONOTONICITY OK\n";

    // --------------------------------------------------------
    // Negative height harus menghasilkan 0
    // --------------------------------------------------------

    if (GetBlockSubsidy(-1) != 0)
    {
        std::cout
            << "NEGATIVE HEIGHT FAIL\n";
        return 1;
    }

    std::cout
        << "NEGATIVE HEIGHT OK\n";

    std::cout
        << "\nALL BERYL CONSENSUS REWARD TESTS PASSED\n";

    return 0;
}
