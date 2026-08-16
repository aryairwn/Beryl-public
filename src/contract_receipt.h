#ifndef BERYL_CONTRACT_RECEIPT_H
#define BERYL_CONTRACT_RECEIPT_H

#include "bvm.h"

#include <cstdint>
#include <string>
#include <vector>

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT RECEIPT V1
//
// Hasil resmi satu kali contract execution.
//
// Receipt tidak mengubah state.
// Receipt hanya membawa hasil deterministic execution:
//
//   - status
//   - gasUsed
//   - returnData
//   - logs
//   - error
// ============================================================

struct ContractReceipt
{
    bvm::Status status = bvm::Status::SUCCESS;

    uint64_t gasUsed = 0;

    std::vector<uint8_t> returnData;

    std::vector<uint8_t> logs;

    std::string error;

    bool Success() const
    {
        return status == bvm::Status::SUCCESS;
    }

    bool Failed() const
    {
        return !Success();
    }

    static ContractReceipt FromBVMResult(
        const bvm::Result& result
    )
    {
        ContractReceipt receipt;

        receipt.status = result.status;
        receipt.gasUsed = result.gasUsed;
        receipt.returnData = result.returnData;
        receipt.logs = result.logs;
        receipt.error = result.error;

        return receipt;
    }
};

} // namespace beryl::contract

#endif
