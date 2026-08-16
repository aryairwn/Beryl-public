// © Arya Irawan — 10 August 2026

#include "merkle.h"
#include "crypto/blake3_wrapper.h"

#include <sstream>
#include <iomanip>

std::string HashPair(
    const std::string& left,
    const std::string& right
)
{
    // Gabungkan dua node secara deterministik.
    std::string combined = left + right;

    std::vector<unsigned char> data(
        combined.begin(),
        combined.end()
    );

    // Merkle node Beryl menggunakan BLAKE3-256.
    std::vector<unsigned char> hash =
        Blake3Hash(data, 32);

    std::stringstream ss;

    for (unsigned char b : hash)
    {
        ss
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(b);
    }

    return ss.str();
}

std::string CalculateMerkleRoot(
    const std::vector<BerylTransaction>& txs
)
{
    if(txs.empty())
    {
        return "";
    }

    std::vector<std::string> hashes;

    for(const auto& tx : txs)
    {
        hashes.push_back(tx.txid);
    }

    while(hashes.size() > 1)
    {
        if(hashes.size() % 2 == 1)
        {
            hashes.push_back(
                hashes.back()
            );
        }

        std::vector<std::string> next;

        for(
            size_t i = 0;
            i < hashes.size();
            i += 2
        )
        {
            next.push_back(
                HashPair(
                    hashes[i],
                    hashes[i + 1]
                )
            );
        }

        hashes = next;
    }

    return hashes.front();
}
