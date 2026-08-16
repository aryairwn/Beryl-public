#include "contract.h"

#include "crypto/blake3_wrapper.h"

#include <algorithm>

namespace beryl::contract {

// ============================================================
// Contract creation
// ============================================================

bool Contract::Create(
    const std::vector<uint8_t>& bytecode,
    std::string& error
)
{
    error.clear();

    if (!ValidateBytecode(bytecode, error))
        return false;

    m_bytecode = bytecode;

    // Code identity is derived from deterministic BLAKE3.
    const std::vector<unsigned char> hash =
        Blake3Hash(
            m_bytecode,
            32
        );

    if (hash.size() != 32)
    {
        error = "Beryl Contract: invalid BLAKE3 hash size";
        m_bytecode.clear();
        m_codeHash.fill(0);
        return false;
    }

    std::copy(
        hash.begin(),
        hash.end(),
        m_codeHash.begin()
    );

    return true;
}

// ============================================================
// Bytecode validation
// ============================================================

bool Contract::ValidateBytecode(
    const std::vector<uint8_t>& bytecode,
    std::string& error
) const
{
    if (bytecode.empty())
    {
        error = "Beryl Contract: empty bytecode";
        return false;
    }

    if (bytecode.size() > MAX_CONTRACT_CODE_SIZE)
    {
        error = "Beryl Contract: bytecode too large";
        return false;
    }

    // Validate instruction boundaries.
    //
    // PUSH V1 consumes:
    //   opcode + 2 bytes value
    //
    // We reject truncated PUSH before execution.
    size_t pc = 0;

    while (pc < bytecode.size())
    {
        const uint8_t opcode = bytecode[pc++];

        size_t operandSize = 0;

        switch (opcode)
        {
            case static_cast<uint8_t>(bvm::Op::PUSH8):
                operandSize = 1;
                break;

            case static_cast<uint8_t>(bvm::Op::PUSH16):
                operandSize = 2;
                break;

            case static_cast<uint8_t>(bvm::Op::PUSH32):
                operandSize = 4;
                break;

            case static_cast<uint8_t>(bvm::Op::PUSH64):
                operandSize = 8;
                break;

            default:
                break;
        }

        if (operandSize != 0)
        {
            if (operandSize > bytecode.size() - pc)
            {
                error = "Beryl Contract: truncated PUSH";
                return false;
            }

            pc += operandSize;
        }
    }

    return true;
}

// ============================================================
// Execution
// ============================================================

bvm::Result Contract::Execute(
    uint64_t gasLimit,
    bvm::Storage* storage
) const
{
    if (m_bytecode.empty())
    {
        bvm::Result result;
        result.status = bvm::Status::REVERT;
        result.error = "Beryl Contract: empty contract";
        return result;
    }

    bvm::VM vm(
        gasLimit,
        storage
    );

    return vm.Execute(m_bytecode);
}

} // namespace beryl::contract
