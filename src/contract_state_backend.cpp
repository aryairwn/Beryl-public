#include "contract_state_backend.h"

#include "crypto/blake3_wrapper.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace beryl::contract {

namespace {

std::string HashBytes(
    const std::vector<uint8_t>& data
)
{
    const std::vector<unsigned char> hash =
        Blake3Hash(data, 32);

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

std::string HashString(
    const std::string& data
)
{
    const std::vector<uint8_t> bytes(
        data.begin(),
        data.end()
    );

    return HashBytes(bytes);
}

void AppendU64(
    std::string& out,
    uint64_t value
)
{
    for (unsigned int i = 0; i < 8; ++i)
    {
        out.push_back(
            static_cast<char>(
                (value >> (8 * i)) & 0xff
            )
        );
    }
}

void AppendContractId(
    std::string& out,
    const ContractId& id
)
{
    for (uint8_t byte : id)
    {
        out.push_back(
            static_cast<char>(byte)
        );
    }
}

} // anonymous namespace

bool ContractStateBackend::Load(
    const ContractId& contractId,
    bvm::Word key,
    bvm::Word& value
)
{
    const auto contractIt =
        m_state.find(contractId);

    if (contractIt == m_state.end())
    {
        value = 0;
        return true;
    }

    const auto keyIt =
        contractIt->second.find(key);

    if (keyIt == contractIt->second.end())
    {
        value = 0;
        return true;
    }

    value = keyIt->second;
    return true;
}

bool ContractStateBackend::Store(
    const ContractId& contractId,
    bvm::Word key,
    bvm::Word value
)
{
    if (!m_transactionOpen)
        return false;

    m_state[contractId][key] = value;

    return true;
}

bool ContractStateBackend::Begin()
{
    if (m_transactionOpen)
        return false;

    m_snapshot = m_state;
    m_transactionOpen = true;

    return true;
}

bool ContractStateBackend::Commit()
{
    if (!m_transactionOpen)
        return false;

    m_snapshot.clear();
    m_transactionOpen = false;

    return true;
}

bool ContractStateBackend::Rollback()
{
    if (!m_transactionOpen)
        return false;

    m_state = m_snapshot;
    m_snapshot.clear();
    m_transactionOpen = false;

    return true;
}

std::string ContractStateBackend::CalculateRoot() const
{
    if (m_state.empty())
    {
        return HashString(
            "BERYL-CONTRACT-STATE-EMPTY"
        );
    }

    std::vector<std::string> leaves;

    for (const auto& contractEntry : m_state)
    {
        const ContractId& contractId =
            contractEntry.first;

        const StateMap& state =
            contractEntry.second;

        if (state.empty())
            continue;

        for (const auto& stateEntry : state)
        {
            std::string data;

            data += "BERYL-CONTRACT-STATE-LEAF|";

            AppendContractId(
                data,
                contractId
            );

            AppendU64(
                data,
                stateEntry.first
            );

            AppendU64(
                data,
                stateEntry.second
            );

            leaves.push_back(
                HashString(data)
            );
        }
    }

    if (leaves.empty())
    {
        return HashString(
            "BERYL-CONTRACT-STATE-EMPTY"
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
                    "BERYL-CONTRACT-STATE-NODE|" +
                    left +
                    right
                )
            );
        }

        leaves = std::move(next);
    }

    return leaves.front();
}

const ContractStateBackend::ContractMap&
ContractStateBackend::GetState() const
{
    return m_state;
}

} // namespace beryl::contract
