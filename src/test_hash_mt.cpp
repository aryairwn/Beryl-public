#include "blockhash.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

int main()
{
    const int THREADS = 8;
    const int HASHES_PER_THREAD = 100;

    std::atomic<uint64_t> total{0};
    std::vector<std::thread> workers;

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < THREADS; ++t)
    {
        workers.emplace_back([&, t]()
        {
            BerylHeader h{};

            h.version = 1;
            h.previousHash = std::string(64, '0');
            h.merkleRoot = std::string(64, '0');
            h.utxoRoot = std::string(64, '0');
            h.timestamp = 1234567890;
            h.difficulty = 1;
            h.height = 1;

            for (int i = 0; i < HASHES_PER_THREAD; ++i)
            {
                h.nonce = static_cast<uint32_t>(
                    t * HASHES_PER_THREAD + i
                );

                GetBlockHash(h);

                total.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
        });
    }

    for (auto& t : workers)
        t.join();

    auto end = std::chrono::steady_clock::now();

    double sec =
        std::chrono::duration<double>(
            end - start
        ).count();

    std::cout << "threads: " << THREADS << "\n";
    std::cout << "hashes: " << total.load() << "\n";
    std::cout << "seconds: " << sec << "\n";
    std::cout << "hashrate: "
              << (total.load() / sec)
              << " H/s\n";

    return 0;
}
