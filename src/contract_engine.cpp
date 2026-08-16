#include "contract_engine.h"

namespace beryl::contract {

// ============================================================
// Context validation
// ============================================================

bool ExecutionEngine::ValidateContext(
    const ExecutionContext& context,
    std::string& error
) const
{
    error.clear();

    if (context.GasLimit() < bvm::MIN_GAS_LIMIT)
    {
        error =
            "Beryl Contract Engine: gas limit too low";
        return false;
    }

    if (context.GasLimit() > bvm::MAX_GAS_LIMIT)
    {
        error =
            "Beryl Contract Engine: gas limit too high";
        return false;
    }

    if (context.Contract() == ContractId{})
    {
        error =
            "Beryl Contract Engine: invalid contract ID";
        return false;
    }

    return true;
}

// ============================================================
// Execute
// ============================================================

bvm::Result ExecutionEngine::Execute(
    const Contract& contract,
    const ExecutionContext& context,
    ContractState& state
)
{
    bvm::Result result;

    std::string error;

    // --------------------------------------------------------
    // Validate execution context
    // --------------------------------------------------------

    if (!ValidateContext(context, error))
    {
        result.status = bvm::Status::REVERT;
        result.error = error;
        return result;
    }

    // --------------------------------------------------------
    // Contract must contain bytecode
    // --------------------------------------------------------

    if (contract.Empty())
    {
        result.status = bvm::Status::REVERT;
        result.error =
            "Beryl Contract Engine: empty contract";
        return result;
    }

    // --------------------------------------------------------
    // State must belong to the executing contract.
    // --------------------------------------------------------

    if (!state.Attached())
    {
        result.status = bvm::Status::REVERT;
        result.error =
            "Beryl Contract Engine: state unavailable";
        return result;
    }

    if (state.Id() != context.Contract())
    {
        result.status = bvm::Status::REVERT;
        result.error =
            "Beryl Contract Engine: contract/state mismatch";
        return result;
    }

    // --------------------------------------------------------
    // Begin atomic state transaction.
    // --------------------------------------------------------

    if (!state.Begin(error))
    {
        result.status = bvm::Status::REVERT;
        result.error = error;
        return result;
    }

    // --------------------------------------------------------
    // Adapt ContractState to the BVM Storage interface.
    //
    // BVM only sees Load/Store.
    // --------------------------------------------------------

    class VMStorage final : public bvm::Storage
    {
    public:
        explicit VMStorage(
            ContractState& state
        )
            : m_state(state)
        {
        }

        bool Load(
            bvm::Word key,
            bvm::Word& value
        ) override
        {
            std::string error;

            return m_state.Load(
                key,
                value,
                error
            );
        }

        bool Store(
            bvm::Word key,
            bvm::Word value
        ) override
        {
            std::string error;

            return m_state.Store(
                key,
                value,
                error
            );
        }

    private:
        ContractState& m_state;
    };

    VMStorage storage(state);

    // --------------------------------------------------------
    // Execute deterministic BVM.
    // --------------------------------------------------------

    bvm::VM vm(
        context.GasLimit(),
        &storage
    );

    result = vm.Execute(
        contract.Bytecode()
    );

    // --------------------------------------------------------
    // Atomic state transition.
    // --------------------------------------------------------

    if (result.Success())
    {
        if (!state.Commit(error))
        {
            // Commit failure means the state transition cannot
            // be accepted as successful.
            //
            // Attempt rollback as a defensive measure.
            std::string rollbackError;
            state.Rollback(rollbackError);

            result.status = bvm::Status::REVERT;
            result.error =
                "Beryl Contract Engine: commit failed";

            return result;
        }

        return result;
    }

    // --------------------------------------------------------
    // Any failed execution rolls back state.
    //
    // This includes:
    //   REVERT
    //   OUT_OF_GAS
    //   INVALID_OPCODE
    //   STACK errors
    //   MEMORY errors
    //   DIVISION_BY_ZERO
    //   INVALID_JUMP
    // --------------------------------------------------------

    std::string rollbackError;

    if (!state.Rollback(rollbackError))
    {
        result.status = bvm::Status::REVERT;
        result.error =
            "Beryl Contract Engine: rollback failed";

        return result;
    }

    return result;
}


// ============================================================
// Execute with ContractReceipt
// ============================================================

ContractReceipt ExecutionEngine::ExecuteWithReceipt(
    const Contract& contract,
    const ExecutionContext& context,
    ContractState& state
)
{
    const bvm::Result result = Execute(
        contract,
        context,
        state
    );

    return ContractReceipt::FromBVMResult(result);
}

} // namespace beryl::contract
