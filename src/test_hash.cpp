#include "blockhash.h"
#include <iostream>
#include <chrono>

int main()
{
    BerylHeader h{};

    h.version = 1;
    h.previousHash = std::string(64, '0');
    h.merkleRoot = std::string(64, '0');
    h.utxoRoot = std::string(64, '0');
    h.timestamp = 1234567890;
    h.difficulty = 1;
    h.height = 1;

    const int N = 100;

    auto start = std::chrono::steady_clock::now();

    std::string last;

    for (int i = 0; i < N; ++i)
    {
        h.nonce = i;
        last = GetBlockHash(h);
    }

    auto end = std::chrono::steady_clock::now();

    double sec =
        std::chrono::duration<double>(end - start).count();

    std::cout << "hashes: " << N << "\n";
    std::cout << "seconds: " << sec << "\n";
    std::cout << "hashrate: " << (N / sec) << " H/s\n";
    std::cout << "last: " << last << "\n";

    return 0;
}
