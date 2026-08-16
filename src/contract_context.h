#ifndef BERYL_CONTRACT_CONTEXT_H
#define BERYL_CONTRACT_CONTEXT_H

#include <cstdint>
#include <string>
#include <vector>

#include "contract_address.h"

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT EXECUTION CONTEXT V1
//
// Context berisi data immutable yang diberikan Core kepada
// contract selama satu execution.
//
// Contract tidak mendapatkan:
//   - private key
//   - seed
//   - wallet object
//   - filesystem
//   - network access
//   - arbitrary system calls
//
// Context harus deterministic.
// ============================================================

class ExecutionContext
{
public:
    ExecutionContext() = default;

    ExecutionContext(
        const std::vector<uint8_t>& caller,
        const ContractId& contract,
        uint64_t gasLimit,
        const std::vector<uint8_t>& input
    );

    bool Initialize(
        const std::vector<uint8_t>& caller,
        const ContractId& contract,
        uint64_t gasLimit,
        const std::vector<uint8_t>& input,
        std::string& error
    );

    const std::vector<uint8_t>& Caller() const
    {
        return m_caller;
    }

    const ContractId& Contract() const
    {
        return m_contract;
    }

    uint64_t GasLimit() const
    {
        return m_gasLimit;
    }

    const std::vector<uint8_t>& Input() const
    {
        return m_input;
    }

private:
    std::vector<uint8_t> m_caller;
    ContractId m_contract{};
    uint64_t m_gasLimit = 0;
    std::vector<uint8_t> m_input;
};

} // namespace beryl::contract

#endif
