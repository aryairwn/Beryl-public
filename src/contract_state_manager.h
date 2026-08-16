#ifndef BERYL_CONTRACT_STATE_MANAGER_H
#define BERYL_CONTRACT_STATE_MANAGER_H

#include "contract_registry.h"
#include "contract_state_backend.h"

#include <string>

namespace beryl::contract {

class ContractStateManager final
{
public:
    ContractStateManager() = default;

    // --------------------------------------------------------
    // Contract registry
    // --------------------------------------------------------

    bool Deploy(
        const std::string& address,
        const Contract& contract
    );

    const Contract* GetContract(
        const std::string& address
    ) const;

    bool HasContract(
        const std::string& address
    ) const;

    // --------------------------------------------------------
    // Contract state
    // --------------------------------------------------------

    ContractStateBackend& Backend()
    {
        return m_backend;
    }

    const ContractStateBackend& Backend() const
    {
        return m_backend;
    }

    // --------------------------------------------------------
    // Deterministic consensus commitment.
    //
    // Root mencakup:
    //   1. contract registry
    //   2. contract storage state
    // --------------------------------------------------------

    std::string CalculateRoot() const;

    // --------------------------------------------------------
    // Reset state.
    // Dipakai saat rebuild blockchain.
    // --------------------------------------------------------

    void Clear();

private:
    ContractRegistry m_registry;
    ContractStateBackend m_backend;
};

} // namespace beryl::contract

#endif
