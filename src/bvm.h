#ifndef BERYL_BVM_H
#define BERYL_BVM_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace beryl::bvm {

// ============================================================
// BVM V1
// Deterministic, stack-based, integer-only VM
// ============================================================

using Word = uint64_t;

static constexpr size_t DEFAULT_MAX_STACK = 1024;
static constexpr size_t MEMORY_PAGE_SIZE = 4096;
static constexpr size_t DEFAULT_MAX_MEMORY = 64 * 1024;

// ============================================================
// BERYL BVM GAS LIMIT V1
// ============================================================
// Gas limit adalah batas maksimum computational execution.
// Nilai ini merupakan bagian dari consensus BVM V1.

static constexpr uint64_t MIN_GAS_LIMIT = 1000;
static constexpr uint64_t MAX_GAS_LIMIT = 10'000'000;

// ============================================================
// Opcodes
// ============================================================

enum class Op : uint8_t
{
    STOP = 0x00,

    PUSH8  = 0x01,
    PUSH16 = 0x02,
    PUSH32 = 0x03,
    PUSH64 = 0x04,

    POP  = 0x05,
    DUP  = 0x06,
    SWAP = 0x07,

    ADD = 0x10,
    SUB = 0x11,
    MUL = 0x12,
    DIV = 0x13,
    MOD = 0x14,

    EQ = 0x20,
    LT = 0x21,
    GT = 0x22,

    AND = 0x30,
    OR  = 0x31,
    XOR = 0x32,
    NOT = 0x33,

    MLOAD  = 0x40,
    MSTORE = 0x41,

    SLOAD  = 0x50,
    SSTORE = 0x51,

    JUMP     = 0x60,
    JZ       = 0x61,
    JUMPDEST = 0x62,

    RETURN = 0x63,
    REVERT = 0x64,

    EMIT = 0x80
};

// ============================================================
// Execution status
// ============================================================

enum class Status
{
    SUCCESS,
    REVERT,
    OUT_OF_GAS,
    INVALID_OPCODE,
    STACK_UNDERFLOW,
    STACK_OVERFLOW,
    MEMORY_LIMIT,
    INVALID_JUMP,
    DIVISION_BY_ZERO,
    INVALID_MEMORY_ACCESS
};

// ============================================================
// Result
// ============================================================

struct Result
{
    Status status = Status::SUCCESS;
    uint64_t gasUsed = 0;

    std::vector<uint8_t> returnData;
    std::vector<uint8_t> logs;

    std::string error;

    bool Success() const
    {
        return status == Status::SUCCESS;
    }
};

// ============================================================
// Persistent contract storage interface
//
// BVM tidak memiliki database sendiri.
// Beryl Core menyediakan implementasi storage
// yang persistent terhadap blockchain state.
// ============================================================

class Storage
{
public:
    virtual ~Storage() = default;

    virtual bool Load(
        Word key,
        Word& value
    ) = 0;

    virtual bool Store(
        Word key,
        Word value
    ) = 0;
};

// ============================================================
// VM
// ============================================================

class VM
{
public:
    VM(
        uint64_t gasLimit,
        Storage* storage = nullptr,
        size_t maxStack = DEFAULT_MAX_STACK,
        size_t maxMemory = DEFAULT_MAX_MEMORY
    );

    Result Execute(
        const std::vector<uint8_t>& bytecode
    );

private:
    bool ChargeGas(uint64_t amount);

    bool Push(Word value);
    bool Pop(Word& value);
    bool Peek(size_t index, Word& value);

    bool EnsureMemory(
        size_t required
    );

    bool ReadMemory(
        size_t offset,
        Word& value
    );

    bool ReadMemoryRange(
        size_t offset,
        size_t size,
        std::vector<uint8_t>& output
    );

    bool WriteMemory(
        size_t offset,
        Word value
    );

    bool ReadU8(
        const std::vector<uint8_t>& code,
        size_t& pc,
        uint8_t& value
    );

    bool ReadU16(
        const std::vector<uint8_t>& code,
        size_t& pc,
        uint16_t& value
    );

    bool ReadU32(
        const std::vector<uint8_t>& code,
        size_t& pc,
        uint32_t& value
    );

    bool ReadU64(
        const std::vector<uint8_t>& code,
        size_t& pc,
        uint64_t& value
    );

    void Fail(
        Status status,
        const char* message
    );

private:
    uint64_t m_gasLimit;
    uint64_t m_gasUsed;

    size_t m_maxStack;
    size_t m_maxMemory;

    Storage* m_storage;

    std::vector<Word> m_stack;
    std::vector<uint8_t> m_memory;

    Result m_result;
    bool m_failed;
};

} // namespace beryl::bvm

#endif
