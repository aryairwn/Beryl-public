#include "contract_state_manager.h"

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

} // anonymous namespace

// ============================================================
// DEPLOY
// ============================================================

bool ContractStateManager::Deploy(
    const std::string& address,
    const Contract& contract
)
{
    return m_registry.Add(
        address,
        contract
    );
}

// ============================================================
// GET CONTRACT
// ============================================================

const Contract*
ContractStateManager::GetContract(
    const std::string& address
) const
{
    return m_registry.Get(address);
}

// ============================================================
// HAS CONTRACT
// ============================================================

bool ContractStateManager::HasContract(
    const std::string& address
) const
{
    return m_registry.Exists(address);
}

// ============================================================
// ROOT
// ============================================================

std::string ContractStateManager::CalculateRoot() const
{
    const std::string registryRoot =
        m_registry.CalculateRoot();

    const std::string stateRoot =
        m_backend.CalculateRoot();

    return HashString(
        "BERYL-CONTRACT-STATE-ROOT|" +
        registryRoot +
        "|" +
        stateRoot
    );
}

// ============================================================
// CLEAR
// ============================================================

void ContractStateManager::Clear()
{
    // StateBackend V1 belum mempunyai Clear().
    //
    // Untuk sekarang object manager dibuat fresh
    // oleh caller ketika rebuild blockchain.
    //
    // Jangan melakukan mutasi parsial di sini.
}

} // namespace beryl::contract
