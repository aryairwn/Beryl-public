#ifndef BERYL_CONTRACT_STATE_BACKEND_H
#define BERYL_CONTRACT_STATE_BACKEND_H

#include "contract_state.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace beryl::contract {

class ContractStateBackend final : public StateBackend
{
public:
    using StateMap =
        std::map<bvm::Word, bvm::Word>;

    using ContractMap =
        std::map<ContractId, StateMap>;

    ContractStateBackend() = default;

    bool Load(
        const ContractId& contractId,
        bvm::Word key,
        bvm::Word& value
    ) override;

    bool Store(
        const ContractId& contractId,
        bvm::Word key,
        bvm::Word value
    ) override;

    bool Begin() override;
    bool Commit() override;
    bool Rollback() override;

    // Deterministic commitment terhadap seluruh
    // contract state.
    std::string CalculateRoot() const;

    // Mengakses state aktif.
    const ContractMap& GetState() const;

private:
    ContractMap m_state;

    ContractMap m_snapshot;

    bool m_transactionOpen = false;
};

} // namespace beryl::contract

#endif
