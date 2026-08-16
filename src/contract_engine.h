#ifndef BERYL_CONTRACT_ENGINE_H
#define BERYL_CONTRACT_ENGINE_H

#include <cstdint>
#include <string>
#include <vector>

#include "bvm.h"
#include "contract.h"
#include "contract_context.h"
#include "contract_state.h"
#include "contract_receipt.h"

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT EXECUTION ENGINE V1
//
// Satu jalur execution resmi:
//
//     Context
//        +
//     Contract
//        +
//     State
//        ↓
//     BVM
//
// Atomic state transition:
//
//     Begin
//       ↓
//     Execute
//       ├── SUCCESS    → Commit
//       └── failure    → Rollback
//
// Engine tidak memiliki wallet/private key.
// ============================================================

class ExecutionEngine
{
public:
    ExecutionEngine() = default;

    bvm::Result Execute(
        const Contract& contract,
        const ExecutionContext& context,
        ContractState& state
    );

    ContractReceipt ExecuteWithReceipt(
        const Contract& contract,
        const ExecutionContext& context,
        ContractState& state
    );

private:
    bool ValidateContext(
        const ExecutionContext& context,
        std::string& error
    ) const;
};

} // namespace beryl::contract

#endif
