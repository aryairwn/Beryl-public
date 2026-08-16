#include "bvm.h"
#include "contract_context.h"

namespace beryl::contract {

// ============================================================
// Constructor
// ============================================================

ExecutionContext::ExecutionContext(
    const std::vector<uint8_t>& caller,
    const ContractId& contract,
    uint64_t gasLimit,
    const std::vector<uint8_t>& input
)
    : m_caller(caller),
      m_contract(contract),
      m_gasLimit(gasLimit),
      m_input(input)
{
}

// ============================================================
// Initialize
// ============================================================

bool ExecutionContext::Initialize(
    const std::vector<uint8_t>& caller,
    const ContractId& contract,
    uint64_t gasLimit,
    const std::vector<uint8_t>& input,
    std::string& error
)
{
    error.clear();

    if (gasLimit < bvm::MIN_GAS_LIMIT)
    {
        error =
            "Beryl Contract Context: gas limit too low";
        return false;
    }

    if (gasLimit > bvm::MAX_GAS_LIMIT)
    {
        error =
            "Beryl Contract Context: gas limit too high";
        return false;
    }

    // Context copies input data.
    //
    // Contract execution must never depend on a caller-owned
    // mutable buffer.
    m_caller = caller;
    m_contract = contract;
    m_gasLimit = gasLimit;
    m_input = input;

    return true;
}

} // namespace beryl::contract
