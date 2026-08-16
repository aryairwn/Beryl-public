#include "bvm.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace beryl::bvm {

// ============================================================
// Gas schedule V1
//
// Beryl BVM V1 consensus gas schedule.
// These values are part of deterministic contract execution.
// Changing them is a consensus change.
// ============================================================

static constexpr uint64_t GAS_STOP   = 0;

static constexpr uint64_t GAS_PUSH8  = 1;
static constexpr uint64_t GAS_PUSH16 = 1;
static constexpr uint64_t GAS_PUSH32 = 2;
static constexpr uint64_t GAS_PUSH64 = 3;

static constexpr uint64_t GAS_POP    = 1;
static constexpr uint64_t GAS_DUP    = 1;
static constexpr uint64_t GAS_SWAP   = 1;

static constexpr uint64_t GAS_ARITH  = 2;
static constexpr uint64_t GAS_CMP    = 2;
static constexpr uint64_t GAS_LOGIC  = 2;

static constexpr uint64_t GAS_MEMORY = 3;

static constexpr uint64_t GAS_SLOAD  = 20;
static constexpr uint64_t GAS_SSTORE = 100;

static constexpr uint64_t GAS_JUMP   = 3;
static constexpr uint64_t GAS_RETURN = 0;
static constexpr uint64_t GAS_REVERT = 0;
static constexpr uint64_t GAS_EMIT   = 50;

// ============================================================

VM::VM(
    uint64_t gasLimit,
    Storage* storage,
    size_t maxStack,
    size_t maxMemory
)
    : m_gasLimit(gasLimit),
      m_gasUsed(0),
      m_maxStack(maxStack),
      m_maxMemory(maxMemory),
      m_storage(storage),
      m_failed(false)
{
    m_stack.reserve(
        std::min(
            maxStack,
            static_cast<size_t>(1024)
        )
    );

    m_memory.reserve(
        std::min(
            maxMemory,
            static_cast<size_t>(64 * 1024)
        )
    );
}

bool VM::ChargeGas(uint64_t amount)
{
    if (amount > m_gasLimit - m_gasUsed)
    {
        Fail(
            Status::OUT_OF_GAS,
            "BVM: out of gas"
        );

        return false;
    }

    m_gasUsed += amount;
    return true;
}

bool VM::Push(Word value)
{
    if (m_stack.size() >= m_maxStack)
    {
        Fail(
            Status::STACK_OVERFLOW,
            "BVM: stack overflow"
        );

        return false;
    }

    m_stack.push_back(value);
    return true;
}

bool VM::Pop(Word& value)
{
    if (m_stack.empty())
    {
        Fail(
            Status::STACK_UNDERFLOW,
            "BVM: stack underflow"
        );

        return false;
    }

    value = m_stack.back();
    m_stack.pop_back();

    return true;
}

bool VM::Peek(size_t index, Word& value)
{
    if (index >= m_stack.size())
    {
        Fail(
            Status::STACK_UNDERFLOW,
            "BVM: stack underflow"
        );

        return false;
    }

    value = m_stack[
        m_stack.size() - 1 - index
    ];

    return true;
}

bool VM::ReadMemory(
    size_t offset,
    Word& value
)
{
    if (offset > m_maxMemory ||
        sizeof(Word) > m_maxMemory - offset)
    {
        Fail(
            Status::INVALID_MEMORY_ACCESS,
            "BVM: invalid memory access"
        );

        return false;
    }

    if (offset + sizeof(Word) > m_memory.size())
    {
        value = 0;
        return true;
    }

    std::memcpy(
        &value,
        m_memory.data() + offset,
        sizeof(Word)
    );

    return true;
}

bool VM::ReadMemoryRange(
    size_t offset,
    size_t size,
    std::vector<uint8_t>& output
)
{
    output.clear();

    if (offset > m_maxMemory ||
        size > m_maxMemory - offset)
    {
        Fail(
            Status::INVALID_MEMORY_ACCESS,
            "BVM: invalid memory range"
        );
        return false;
    }

    if (size == 0)
        return true;

    output.resize(size, 0);

    if (offset >= m_memory.size())
        return true;

    const size_t available =
        std::min(
            size,
            m_memory.size() - offset
        );

    if (available > 0)
    {
        std::memcpy(
            output.data(),
            m_memory.data() + offset,
            available
        );
    }

    return true;
}

bool VM::WriteMemory(
    size_t offset,
    Word value
)
{
    if (offset > m_maxMemory ||
        sizeof(Word) > m_maxMemory - offset)
    {
        Fail(
            Status::MEMORY_LIMIT,
            "BVM: memory limit"
        );

        return false;
    }

    const size_t required =
        offset + sizeof(Word);

    if (required > m_memory.size())
    {
        m_memory.resize(required, 0);
    }

    std::memcpy(
        m_memory.data() + offset,
        &value,
        sizeof(Word)
    );

    return true;
}

bool VM::ReadU8(
    const std::vector<uint8_t>& code,
    size_t& pc,
    uint8_t& value
)
{
    if (pc >= code.size())
    {
        Fail(
            Status::INVALID_OPCODE,
            "BVM: truncated PUSH8"
        );

        return false;
    }

    value = code[pc++];
    return true;
}

bool VM::ReadU16(
    const std::vector<uint8_t>& code,
    size_t& pc,
    uint16_t& value
)
{
    if (code.size() - pc < 2)
    {
        Fail(
            Status::INVALID_OPCODE,
            "BVM: truncated PUSH16"
        );

        return false;
    }

    value =
        static_cast<uint16_t>(code[pc]) |
        (static_cast<uint16_t>(code[pc + 1]) << 8);

    pc += 2;
    return true;
}

bool VM::ReadU32(
    const std::vector<uint8_t>& code,
    size_t& pc,
    uint32_t& value
)
{
    if (code.size() - pc < 4)
    {
        Fail(
            Status::INVALID_OPCODE,
            "BVM: truncated PUSH32"
        );

        return false;
    }

    value =
        static_cast<uint32_t>(code[pc]) |
        (static_cast<uint32_t>(code[pc + 1]) << 8) |
        (static_cast<uint32_t>(code[pc + 2]) << 16) |
        (static_cast<uint32_t>(code[pc + 3]) << 24);

    pc += 4;
    return true;
}

bool VM::ReadU64(
    const std::vector<uint8_t>& code,
    size_t& pc,
    uint64_t& value
)
{
    if (code.size() - pc < 8)
    {
        Fail(
            Status::INVALID_OPCODE,
            "BVM: truncated PUSH64"
        );

        return false;
    }

    value =
        static_cast<uint64_t>(code[pc]) |
        (static_cast<uint64_t>(code[pc + 1]) << 8) |
        (static_cast<uint64_t>(code[pc + 2]) << 16) |
        (static_cast<uint64_t>(code[pc + 3]) << 24) |
        (static_cast<uint64_t>(code[pc + 4]) << 32) |
        (static_cast<uint64_t>(code[pc + 5]) << 40) |
        (static_cast<uint64_t>(code[pc + 6]) << 48) |
        (static_cast<uint64_t>(code[pc + 7]) << 56);

    pc += 8;
    return true;
}

static bool IsValidJumpDestination(
    const std::vector<uint8_t>& code,
    size_t destination
)
{
    if (destination >= code.size())
        return false;

    size_t pc = 0;

    while (pc < code.size())
    {
        const uint8_t opcode = code[pc];

        if (pc == destination)
            return opcode ==
                static_cast<uint8_t>(Op::JUMPDEST);

        ++pc;

        switch (
            static_cast<Op>(opcode)
        )
        {
            case Op::PUSH8:
                if (code.size() - pc < 1)
                    return false;
                pc += 1;
                break;

            case Op::PUSH16:
                if (code.size() - pc < 2)
                    return false;
                pc += 2;
                break;

            case Op::PUSH32:
                if (code.size() - pc < 4)
                    return false;
                pc += 4;
                break;

            case Op::PUSH64:
                if (code.size() - pc < 8)
                    return false;
                pc += 8;
                break;

            default:
                break;
        }
    }

    return false;
}

void VM::Fail(
    Status status,
    const char* message
)
{
    if (!m_failed)
    {
        m_result.status = status;
        m_result.error = message;
        m_failed = true;
    }
}

// ============================================================
// Execute
// ============================================================

Result VM::Execute(
    const std::vector<uint8_t>& bytecode
)
{
    m_result = Result{};
    m_result.status = Status::SUCCESS;
    m_result.gasUsed = 0;

    m_gasUsed = 0;
    m_failed = false;

    m_stack.clear();
    m_memory.clear();

    size_t pc = 0;

    while (pc < bytecode.size())
    {
        const uint8_t raw = bytecode[pc++];

        const Op op =
            static_cast<Op>(raw);

        uint64_t gas = 0;

        switch (op)
        {
            case Op::STOP:
                gas = GAS_STOP;

                if (!ChargeGas(gas))
                    break;

                m_result.gasUsed = m_gasUsed;
                return m_result;

            case Op::PUSH8:
            {
                if (!ChargeGas(GAS_PUSH8))
                    break;

                uint8_t value = 0;

                if (!ReadU8(
                        bytecode,
                        pc,
                        value))
                    break;

                Push(
                    static_cast<Word>(value)
                );

                break;
            }

            case Op::PUSH16:
            {
                if (!ChargeGas(GAS_PUSH16))
                    break;

                uint16_t value = 0;

                if (!ReadU16(
                        bytecode,
                        pc,
                        value))
                    break;

                Push(
                    static_cast<Word>(value)
                );

                break;
            }

            case Op::PUSH32:
            {
                if (!ChargeGas(GAS_PUSH32))
                    break;

                uint32_t value = 0;

                if (!ReadU32(
                        bytecode,
                        pc,
                        value))
                    break;

                Push(
                    static_cast<Word>(value)
                );

                break;
            }

            case Op::PUSH64:
            {
                if (!ChargeGas(GAS_PUSH64))
                    break;

                uint64_t value = 0;

                if (!ReadU64(
                        bytecode,
                        pc,
                        value))
                    break;

                Push(value);
                break;
            }

            case Op::POP:
            {
                gas = GAS_POP;

                if (!ChargeGas(gas))
                    break;

                Word value = 0;

                if (!Pop(value))
                    break;

                break;
            }

            case Op::DUP:
            {
                gas = GAS_DUP;

                if (!ChargeGas(gas))
                    break;

                Word value = 0;

                if (!Peek(0, value))
                    break;

                if (!Push(value))
                    break;

                break;
            }

            case Op::SWAP:
            {
                gas = GAS_SWAP;

                if (!ChargeGas(gas))
                    break;

                if (m_stack.size() < 2)
                {
                    Fail(
                        Status::STACK_UNDERFLOW,
                        "BVM: stack underflow"
                    );
                    break;
                }

                std::swap(
                    m_stack[m_stack.size() - 1],
                    m_stack[m_stack.size() - 2]
                );

                break;
            }

            case Op::ADD:
            case Op::SUB:
            case Op::MUL:
            case Op::DIV:
            case Op::MOD:
            {
                gas = GAS_ARITH;

                if (!ChargeGas(gas))
                    break;

                Word a = 0;
                Word b = 0;

                if (!Pop(a) || !Pop(b))
                    break;

                Word result = 0;

                if (op == Op::ADD)
                    result = b + a;

                else if (op == Op::SUB)
                    result = b - a;

                else if (op == Op::MUL)
                    result = b * a;

                else if (op == Op::DIV)
                {
                    if (a == 0)
                    {
                        Fail(
                            Status::DIVISION_BY_ZERO,
                            "BVM: division by zero"
                        );
                        break;
                    }

                    result = b / a;
                }

                else
                {
                    if (a == 0)
                    {
                        Fail(
                            Status::DIVISION_BY_ZERO,
                            "BVM: modulo by zero"
                        );
                        break;
                    }

                    result = b % a;
                }

                if (!m_failed)
                    Push(result);

                break;
            }

            case Op::EQ:
            case Op::LT:
            case Op::GT:
            {
                gas = GAS_CMP;

                if (!ChargeGas(gas))
                    break;

                Word a = 0;
                Word b = 0;

                if (!Pop(a) || !Pop(b))
                    break;

                Word result = 0;

                if (op == Op::EQ)
                    result = (b == a);

                else if (op == Op::LT)
                    result = (b < a);

                else
                    result = (b > a);

                Push(result);
                break;
            }

            case Op::AND:
            case Op::OR:
            case Op::XOR:
            {
                gas = GAS_LOGIC;

                if (!ChargeGas(gas))
                    break;

                Word a = 0;
                Word b = 0;

                if (!Pop(a) || !Pop(b))
                    break;

                Word result = 0;

                if (op == Op::AND)
                    result = b & a;

                else if (op == Op::OR)
                    result = b | a;

                else
                    result = b ^ a;

                Push(result);
                break;
            }

            case Op::NOT:
            {
                gas = GAS_LOGIC;

                if (!ChargeGas(gas))
                    break;

                Word value = 0;

                if (!Pop(value))
                    break;

                Push(~value);
                break;
            }

            case Op::MLOAD:
            {
                gas = GAS_MEMORY;

                if (!ChargeGas(gas))
                    break;

                Word offset = 0;

                if (!Pop(offset))
                    break;

                Word value = 0;

                if (!ReadMemory(
                        static_cast<size_t>(offset),
                        value))
                    break;

                Push(value);
                break;
            }

            case Op::MSTORE:
            {
                gas = GAS_MEMORY;

                if (!ChargeGas(gas))
                    break;

                Word offset = 0;
                Word value = 0;

                if (!Pop(offset) ||
                    !Pop(value))
                    break;

                if (!WriteMemory(
                        static_cast<size_t>(offset),
                        value))
                    break;

                break;
            }

            case Op::SLOAD:
            {
                if (!ChargeGas(GAS_SLOAD))
                    break;

                if (m_storage == nullptr)
                {
                    Fail(
                        Status::REVERT,
                        "BVM: storage unavailable"
                    );
                    break;
                }

                Word key = 0;

                if (!Pop(key))
                    break;

                Word value = 0;

                if (!m_storage->Load(
                        key,
                        value))
                {
                    Fail(
                        Status::REVERT,
                        "BVM: storage read failed"
                    );
                    break;
                }

                if (!Push(value))
                    break;

                break;
            }

            case Op::SSTORE:
            {
                if (!ChargeGas(GAS_SSTORE))
                    break;

                if (m_storage == nullptr)
                {
                    Fail(
                        Status::REVERT,
                        "BVM: storage unavailable"
                    );
                    break;
                }

                Word key = 0;
                Word value = 0;

                // Stack: ... key value
                // SSTORE consumes value first, then key.

                if (!Pop(value) ||
                    !Pop(key))
                    break;

                if (!m_storage->Store(
                        key,
                        value))
                {
                    Fail(
                        Status::REVERT,
                        "BVM: storage write failed"
                    );
                    break;
                }

                break;
            }

            case Op::JUMP:
            {
                gas = GAS_JUMP;

                if (!ChargeGas(gas))
                    break;

                Word destination = 0;

                if (!Pop(destination))
                    break;

                const size_t target =
                    static_cast<size_t>(destination);

                if (!IsValidJumpDestination(
                        bytecode,
                        target))
                {
                    Fail(
                        Status::INVALID_JUMP,
                        "BVM: invalid jump destination"
                    );
                    break;
                }

                pc = target;

                break;
            }

            case Op::JZ:
            {
                gas = GAS_JUMP;

                if (!ChargeGas(gas))
                    break;

                Word destination = 0;
                Word condition = 0;

                if (!Pop(condition) ||
                    !Pop(destination))
                    break;

                if (condition == 0)
                {
                    const size_t target =
                        static_cast<size_t>(destination);

                    if (!IsValidJumpDestination(
                            bytecode,
                            target))
                    {
                        Fail(
                            Status::INVALID_JUMP,
                            "BVM: invalid jump destination"
                        );
                        break;
                    }

                    pc = target;
                }

                break;
            }

            case Op::JUMPDEST:
            {
                if (!ChargeGas(GAS_JUMP))
                    break;

                break;
            }

            case Op::RETURN:
            {
                gas = GAS_RETURN;

                if (!ChargeGas(gas))
                    break;

                Word size = 0;
                Word offset = 0;

                // Stack:
                //   ... offset size
                //
                // Top of stack = size.
                if (!Pop(size) || !Pop(offset))
                    break;

                if (size > m_maxMemory ||
                    offset > m_maxMemory - size)
                {
                    Fail(
                        Status::INVALID_MEMORY_ACCESS,
                        "BVM: invalid return range"
                    );
                    break;
                }

                if (!ReadMemoryRange(
                        static_cast<size_t>(offset),
                        static_cast<size_t>(size),
                        m_result.returnData
                    ))
                    break;

                m_result.gasUsed = m_gasUsed;
                return m_result;
            }

            case Op::REVERT:
            {
                gas = GAS_REVERT;

                if (!ChargeGas(gas))
                    break;

                Word size = 0;
                Word offset = 0;

                // Stack:
                //   ... offset size
                //
                // Top of stack = size.
                if (!Pop(size) || !Pop(offset))
                    break;

                if (size > m_maxMemory ||
                    offset > m_maxMemory - size)
                {
                    Fail(
                        Status::INVALID_MEMORY_ACCESS,
                        "BVM: invalid revert range"
                    );
                    break;
                }

                if (!ReadMemoryRange(
                        static_cast<size_t>(offset),
                        static_cast<size_t>(size),
                        m_result.returnData
                    ))
                    break;

                Fail(
                    Status::REVERT,
                    "BVM: execution reverted"
                );

                break;
            }

            case Op::EMIT:
            {
                Word size = 0;
                Word offset = 0;

                // Stack:
                //   ... offset size
                //
                // Top of stack = size.
                if (!Pop(size) || !Pop(offset))
                    break;

                // The emitted range must stay inside the
                // deterministic VM memory limit.
                if (size > m_maxMemory ||
                    offset > m_maxMemory - size)
                {
                    Fail(
                        Status::INVALID_MEMORY_ACCESS,
                        "BVM: invalid emit range"
                    );
                    break;
                }

                // V1 gas:
                //   base cost + one gas unit per emitted byte.
                //
                // m_maxMemory is bounded, therefore the addition
                // cannot overflow uint64_t here.
                gas = GAS_EMIT + size;

                if (!ChargeGas(gas))
                    break;

                std::vector<uint8_t> data;

                if (!ReadMemoryRange(
                        static_cast<size_t>(offset),
                        static_cast<size_t>(size),
                        data
                    ))
                    break;

                // One EMIT = one deterministic log record.
                //
                // Encoding:
                //   [uint64 little-endian payload size]
                //   [payload bytes]
                //
                // A fixed-width length prefix keeps multiple EMIT
                // records unambiguously separable.
                const uint64_t length = size;

                for (unsigned int i = 0; i < 8; ++i)
                {
                    m_result.logs.push_back(
                        static_cast<uint8_t>((length >> (i * 8)) & 0xff)
                    );
                }

                m_result.logs.insert(
                    m_result.logs.end(),
                    data.begin(),
                    data.end()
                );

                break;
            }

            default:
                Fail(
                    Status::INVALID_OPCODE,
                    "BVM: invalid opcode"
                );

                break;
        }

        if (m_failed)
            break;
    }

    m_result.gasUsed = m_gasUsed;

    return m_result;
}

} // namespace beryl::bvm
