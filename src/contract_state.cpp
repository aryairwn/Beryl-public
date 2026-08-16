#include "contract_state.h"

namespace beryl::contract {

// ============================================================
// Constructor
// ============================================================

ContractState::ContractState(
    const ContractId& contractId,
    StateBackend* backend
)
    : m_contractId(contractId),
      m_backend(backend)
{
}

// ============================================================
// Attach
// ============================================================

bool ContractState::Attach(
    const ContractId& contractId,
    StateBackend* backend,
    std::string& error
)
{
    error.clear();

    if (backend == nullptr)
    {
        error = "Beryl Contract State: null backend";
        return false;
    }

    if (m_transactionOpen)
    {
        error =
            "Beryl Contract State: transaction already open";
        return false;
    }

    m_contractId = contractId;
    m_backend = backend;

    return true;
}

// ============================================================
// Load
// ============================================================

bool ContractState::Load(
    bvm::Word key,
    bvm::Word& value,
    std::string& error
)
{
    error.clear();

    if (m_backend == nullptr)
    {
        error =
            "Beryl Contract State: backend unavailable";
        return false;
    }

    if (!m_backend->Load(
            m_contractId,
            key,
            value))
    {
        error =
            "Beryl Contract State: storage read failed";
        return false;
    }

    return true;
}

// ============================================================
// Store
// ============================================================

bool ContractState::Store(
    bvm::Word key,
    bvm::Word value,
    std::string& error
)
{
    error.clear();

    if (m_backend == nullptr)
    {
        error =
            "Beryl Contract State: backend unavailable";
        return false;
    }

    if (!m_transactionOpen)
    {
        error =
            "Beryl Contract State: Store outside transaction";
        return false;
    }

    if (!m_backend->Store(
            m_contractId,
            key,
            value))
    {
        error =
            "Beryl Contract State: storage write failed";
        return false;
    }

    return true;
}

// ============================================================
// Begin
// ============================================================

bool ContractState::Begin(
    std::string& error
)
{
    error.clear();

    if (m_backend == nullptr)
    {
        error =
            "Beryl Contract State: backend unavailable";
        return false;
    }

    if (m_transactionOpen)
    {
        error =
            "Beryl Contract State: transaction already open";
        return false;
    }

    if (!m_backend->Begin())
    {
        error =
            "Beryl Contract State: begin failed";
        return false;
    }

    m_transactionOpen = true;

    return true;
}

// ============================================================
// Commit
// ============================================================

bool ContractState::Commit(
    std::string& error
)
{
    error.clear();

    if (m_backend == nullptr)
    {
        error =
            "Beryl Contract State: backend unavailable";
        return false;
    }

    if (!m_transactionOpen)
    {
        error =
            "Beryl Contract State: no transaction";
        return false;
    }

    if (!m_backend->Commit())
    {
        error =
            "Beryl Contract State: commit failed";
        return false;
    }

    m_transactionOpen = false;

    return true;
}

// ============================================================
// Rollback
// ============================================================

bool ContractState::Rollback(
    std::string& error
)
{
    error.clear();

    if (m_backend == nullptr)
    {
        error =
            "Beryl Contract State: backend unavailable";
        return false;
    }

    if (!m_transactionOpen)
    {
        error =
            "Beryl Contract State: no transaction";
        return false;
    }

    if (!m_backend->Rollback())
    {
        error =
            "Beryl Contract State: rollback failed";
        return false;
    }

    m_transactionOpen = false;

    return true;
}

} // namespace beryl::contract
