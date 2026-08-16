#ifndef BERYL_CONTRACT_STATE_H
#define BERYL_CONTRACT_STATE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "bvm.h"
#include "contract_address.h"

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT STATE V1
//
// State contract dimiliki oleh Beryl Core.
//
// BVM hanya meminta:
//
//     Load(key)
//     Store(key, value)
//
// VM tidak memiliki database blockchain sendiri.
//
// State transition harus bersifat:
//     deterministic
//     bounded
//     atomic
//
// Snapshot/Rollback disediakan agar perubahan contract dapat
// dibatalkan apabila execution berakhir REVERT / OUT_OF_GAS.
// ============================================================

class StateBackend
{
public:
    virtual ~StateBackend() = default;

    virtual bool Load(
        const ContractId& contractId,
        bvm::Word key,
        bvm::Word& value
    ) = 0;

    virtual bool Store(
        const ContractId& contractId,
        bvm::Word key,
        bvm::Word value
    ) = 0;

    // Begin an execution-level state transaction.
    virtual bool Begin() = 0;

    // Commit all changes made since Begin().
    virtual bool Commit() = 0;

    // Discard all changes made since Begin().
    virtual bool Rollback() = 0;
};

// ============================================================
// Contract State
// ============================================================

class ContractState
{
public:
    ContractState() = default;

    ContractState(
        const ContractId& contractId,
        StateBackend* backend
    );

    bool Attach(
        const ContractId& contractId,
        StateBackend* backend,
        std::string& error
    );

    bool Load(
        bvm::Word key,
        bvm::Word& value,
        std::string& error
    );

    bool Store(
        bvm::Word key,
        bvm::Word value,
        std::string& error
    );

    bool Begin(
        std::string& error
    );

    bool Commit(
        std::string& error
    );

    bool Rollback(
        std::string& error
    );

    const ContractId& Id() const
    {
        return m_contractId;
    }

    bool Attached() const
    {
        return m_backend != nullptr;
    }

private:
    ContractId m_contractId{};
    StateBackend* m_backend = nullptr;
    bool m_transactionOpen = false;
};

} // namespace beryl::contract

#endif
