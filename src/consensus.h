// © Arya Irawan — 10 August 2026

#ifndef BERYL_CONSENSUS_H
#define BERYL_CONSENSUS_H

#include <cstdint>


namespace BerylConsensus
{

static const char* NAME = "Beryl";

static const char* SYMBOL = "BER";


// block time 6 detik
static const uint32_t BLOCK_TIME = 6;

// Coinbase reward baru dapat dibelanjakan setelah 10 blok.
static constexpr uint32_t COINBASE_MATURITY = 10;

// difficulty disesuaikan setiap 600 blok (±1 jam)
static constexpr uint32_t DIFFICULTY_ADJUSTMENT_INTERVAL = 100;

// supply maksimal



// reward awal
static constexpr uint64_t COIN = 100000000ULL;

    // Supply maksimal = 2.100.000.000 BER.
    static constexpr uint64_t MAX_SUPPLY =
        2100000000ULL * COIN;

    // Reward awal = 40 BER.
    static constexpr uint64_t INITIAL_REWARD =
        40ULL * COIN;

    // Reward menjadi 90% dari tahun sebelumnya.
    // Artinya turun 10% per tahun secara compound.
    static constexpr uint64_t REWARD_NUMERATOR = 9;
    static constexpr uint64_t REWARD_DENOMINATOR = 10;

    // 365 hari dengan block time 6 detik.
    static constexpr uint64_t BLOCKS_PER_YEAR =
        (365ULL * 24ULL * 60ULL * 60ULL) / BLOCK_TIME;


// reward turun 90% setiap tahun




// ========================================================
// ========================================================
// PROOF OF WORK BERYL
// ========================================================
//
// difficulty adalah TARGET numerik 64-bit.
//
// Sebuah hash valid jika:
//     hash64 <= difficulty
//
// Target lebih besar  = lebih mudah.
// Target lebih kecil  = lebih sulit.
//
// Target awal dirancang untuk sekitar 600 expected hashes.
// Dengan 100 H/s:
//
//     600 / 100 = 6 detik.
//
// Difficulty kemudian disesuaikan berdasarkan waktu block
// aktual agar rata-rata block time mendekati 6 detik.
//
// ========================================================

static constexpr uint64_t INITIAL_POW_TARGET =
    UINT64_MAX / 600ULL;

// Legacy constant tidak lagi dipakai untuk chain baru.
static constexpr uint32_t INITIAL_DIFFICULTY = 1;

// BLOCK SIZE LIMIT
    // ========================================================
    // Ukuran maksimum serialized block yang diperbolehkan
    // oleh consensus Beryl.
    //
    // 8 MiB dipilih agar transaksi Falcon memiliki ruang
    // yang cukup tanpa membuat block tidak terkendali.
    // ========================================================
    static constexpr uint64_t MAX_BLOCK_SIZE =
        8ULL * 1024ULL * 1024ULL;

    // Block subsidy berdasarkan tinggi blok.
    uint64_t GetBlockSubsidy(int height);


}

#endif
