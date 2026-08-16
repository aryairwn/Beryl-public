// © Arya Irawan — 10 August 2026

#include "utxoroot.h"
#include "crypto/blake3_wrapper.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{

static std::string HashData(const std::string& data)
{
    const std::vector<unsigned char> input(
        data.begin(),
        data.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(input, 32);

    if (hash.size() != 32)
        return "";

    std::ostringstream ss;

    for (const unsigned char byte : hash)
    {
        ss << std::hex
           << std::setw(2)
           << std::setfill('0')
           << static_cast<unsigned int>(byte);
    }

    return ss.str();
}

static std::string SerializeUTXO(const UTXO& u)
{
    std::ostringstream ss;

    ss << u.txid
       << '|'
       << u.index
       << '|'
       << u.address
       << '|'
       << u.amount
       << '|'
       << u.height
       << '|'
       << (u.coinbase ? 1 : 0);

    return ss.str();
}

}

std::string CalculateUTXORoot(
    const UTXOManager& utxoManager
)
{
    std::vector<UTXO> utxos;

    for (const auto& u : utxoManager.GetAllUTXOs())
        utxos.push_back(u);

    std::sort(
        utxos.begin(),
        utxos.end(),
        [](const UTXO& a, const UTXO& b)
        {
            if (a.txid != b.txid)
                return a.txid < b.txid;

            return a.index < b.index;
        }
    );

    if (utxos.empty())
        return HashData("BERYL-UTXO-EMPTY");

    std::vector<std::string> level;

    for (const auto& u : utxos)
    {
        const std::string leaf =
            "BERYL-UTXO-LEAF|" + SerializeUTXO(u);

        level.push_back(HashData(leaf));
    }

    while (level.size() > 1)
    {
        std::vector<std::string> next;

        for (size_t i = 0; i < level.size(); i += 2)
        {
            const std::string& left = level[i];

            const std::string& right =
                (i + 1 < level.size())
                    ? level[i + 1]
                    : level[i];

            next.push_back(
                HashData(
                    "BERYL-UTXO-NODE|" +
                    left +
                    right
                )
            );
        }

        level = std::move(next);
    }

    return level.front();
}
