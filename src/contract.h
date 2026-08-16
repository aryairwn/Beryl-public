#ifndef BERYL_CONTRACT_H
#define BERYL_CONTRACT_H

#include "bvm.h"
#include "contract_address.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace beryl::contract {

// ============================================================
// BERYL CONTRACT V1
//
// Contract layer berada di atas BVM.
//
// Contract:
//   - memiliki bytecode
//   - memiliki code hash
//   - menggunakan BVM untuk execution
//   - menggunakan Storage milik Core
//   - tidak memiliki private key
//   - tidak memiliki wallet
//
// ContractId didefinisikan SATU KALI saja di:
//     contract_address.h
// ============================================================

static constexpr size_t MAX_CONTRACT_CODE_SIZE = 256 * 1024;

// ============================================================
// Contract
// ============================================================

class Contract
{
public:
    Contract() = default;

    // Create contract from validated bytecode.
    bool Create(
        const std::vector<uint8_t>& bytecode,
        std::string& error
    );

    // Execute contract bytecode through BVM.
    bvm::Result Execute(
        uint64_t gasLimit,
        bvm::Storage* storage
    ) const;

    const std::vector<uint8_t>& Bytecode() const
    {
        return m_bytecode;
    }

    const ContractId& CodeHash() const
    {
        return m_codeHash;
    }

    bool Empty() const
    {
        return m_bytecode.empty();
    }

private:
    bool ValidateBytecode(
        const std::vector<uint8_t>& bytecode,
        std::string& error
    ) const;

    std::vector<uint8_t> m_bytecode;
    ContractId m_codeHash{};
};

} // namespace beryl::contract

#endif
