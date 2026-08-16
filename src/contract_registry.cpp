#include "contract_registry.h"

#include "crypto/blake3_wrapper.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace beryl::contract {

namespace {

std::string HashString(
    const std::string& data
)
{
    const std::vector<unsigned char> input(
        data.begin(),
        data.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(input, 32);

    if (hash.size() != 32)
        return {};

    std::ostringstream ss;

    for (unsigned char byte : hash)
    {
        ss << std::hex
           << std::setw(2)
           << std::setfill('0')
           << static_cast<unsigned int>(byte);
    }

    return ss.str();
}

void AppendU32(
    std::string& out,
    uint32_t value
)
{
    for (unsigned int i = 0; i < 4; ++i)
    {
        out.push_back(
            static_cast<char>(
                (value >> (8 * i)) & 0xff
            )
        );
    }
}

} // anonymous namespace

bool ContractRegistry::Add(
    const std::string& address,
    const Contract& contract
)
{
    if (address.empty())
        return false;

    if (contract.Empty())
        return false;

    if (m_contracts.find(address) != m_contracts.end())
        return false;

    m_contracts.emplace(
        address,
        contract
    );

    return true;
}

bool ContractRegistry::Exists(
    const std::string& address
) const
{
    return m_contracts.find(address) !=
           m_contracts.end();
}

const Contract* ContractRegistry::Get(
    const std::string& address
) const
{
    const auto it =
        m_contracts.find(address);

    if (it == m_contracts.end())
        return nullptr;

    return &it->second;
}

const ContractRegistry::RegistryMap&
ContractRegistry::GetAll() const
{
    return m_contracts;
}

std::string ContractRegistry::CalculateRoot() const
{
    if (m_contracts.empty())
    {
        return HashString(
            "BERYL-CONTRACT-REGISTRY-EMPTY"
        );
    }

    std::vector<std::string> leaves;

    for (const auto& entry : m_contracts)
    {
        const std::string& address =
            entry.first;

        const Contract& contract =
            entry.second;

        std::string data;

        data +=
            "BERYL-CONTRACT-REGISTRY-LEAF|";

        AppendU32(
            data,
            static_cast<uint32_t>(
                address.size()
            )
        );

        data += address;

        for (uint8_t byte : contract.CodeHash())
        {
            data.push_back(
                static_cast<char>(byte)
            );
        }

        AppendU32(
            data,
            static_cast<uint32_t>(
                contract.Bytecode().size()
            )
        );

        for (uint8_t byte :
             contract.Bytecode())
        {
            data.push_back(
                static_cast<char>(byte)
            );
        }

        leaves.push_back(
            HashString(data)
        );
    }

    while (leaves.size() > 1)
    {
        std::vector<std::string> next;

        for (size_t i = 0;
             i < leaves.size();
             i += 2)
        {
            const std::string& left =
                leaves[i];

            const std::string& right =
                (i + 1 < leaves.size())
                    ? leaves[i + 1]
                    : leaves[i];

            next.push_back(
                HashString(
                    "BERYL-CONTRACT-REGISTRY-NODE|" +
                    left +
                    right
                )
            );
        }

        leaves = std::move(next);
    }

    return leaves.front();
}

} // namespace beryl::contract
