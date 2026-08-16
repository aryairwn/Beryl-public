#include <iostream>
// © Arya Irawan — 10 August 2026

#include "storage.h"
#include "blockvalidation.h"

#include <fstream>
#include <cstdint>

static bool WriteString(
    std::ofstream& out,
    const std::string& value
)
{
    uint64_t size = value.size();

    out.write(
        reinterpret_cast<const char*>(&size),
        sizeof(size)
    );

    if (!out)
        return false;

    if (size > 0)
    {
        out.write(
            value.data(),
            static_cast<std::streamsize>(size)
        );
    }

    return static_cast<bool>(out);
}

static bool ReadString(
    std::ifstream& in,
    std::string& value
)
{
    uint64_t size = 0;

    in.read(
        reinterpret_cast<char*>(&size),
        sizeof(size)
    );

    if (!in)
        return false;

    // Proteksi ukuran data yang tidak masuk akal.
    if (size > 64ULL * 1024ULL * 1024ULL)
        return false;

    value.resize(
        static_cast<size_t>(size)
    );

    if (size > 0)
    {
        in.read(
            &value[0],
            static_cast<std::streamsize>(size)
        );
    }

    return static_cast<bool>(in);
}

bool SaveBlockchain(
    const BerylChain& chain,
    const std::string& filename
)
{
    std::ofstream out(
        filename,
        std::ios::binary |
        std::ios::trunc
    );

    if (!out)
        return false;

    // Magic.
    const char magic[] = "BERYLCHAIN3";

    out.write(
        magic,
        sizeof(magic) - 1
    );

    // Jumlah block.
    uint64_t blockCount =
        chain.GetBlocks().size();

    out.write(
        reinterpret_cast<const char*>(&blockCount),
        sizeof(blockCount)
    );

    if (!out)
        return false;

    for (const auto& block : chain.GetBlocks())
    {
        // ============================
        // HEADER
        // ============================

        out.write(
            reinterpret_cast<const char*>(
                &block.header.version
            ),
            sizeof(block.header.version)
        );

        if (!WriteString(
                out,
                block.header.previousHash))
            return false;

        if (!WriteString(
                out,
                block.header.merkleRoot))
            return false;

        if (!WriteString(
                out,
                block.header.utxoRoot))
            return false;

        if (!WriteString(
                out,
                block.header.contractRoot))
            return false;

        out.write(
            reinterpret_cast<const char*>(
                &block.header.timestamp
            ),
            sizeof(block.header.timestamp)
        );

        out.write(
            reinterpret_cast<const char*>(
                &block.header.nonce
            ),
            sizeof(block.header.nonce)
        );

        out.write(
            reinterpret_cast<const char*>(
                &block.header.difficulty
            ),
            sizeof(block.header.difficulty)
        );

        out.write(
            reinterpret_cast<const char*>(
                &block.header.height
            ),
            sizeof(block.header.height)
        );

        // ============================
        // BLOCK
        // ============================

        out.write(
            reinterpret_cast<const char*>(
                &block.reward
            ),
            sizeof(block.reward)
        );

        if (!WriteString(
                out,
                block.hash))
            return false;

        // ============================
        // TRANSACTIONS
        // ============================

        uint64_t txCount =
            block.transactions.size();

        out.write(
            reinterpret_cast<const char*>(&txCount),
            sizeof(txCount)
        );

        for (const auto& tx : block.transactions)
    {
        if (!WriteString(
                out,
                tx.txid))
            return false;

        
            // ========================================================
            // TRANSACTION TYPE
            // ========================================================
            uint8_t txType =
                static_cast<uint8_t>(tx.type);

            out.write(
                reinterpret_cast<const char*>(&txType),
                sizeof(txType)
            );

            // ========================================================
            // CONTRACT TYPE
            // ========================================================
            uint8_t contractType =
                static_cast<uint8_t>(tx.contract.type);

            out.write(
                reinterpret_cast<const char*>(&contractType),
                sizeof(contractType)
            );

            // ========================================================
            // CONTRACT CALLER
            // ========================================================
            uint64_t callerSize =
                static_cast<uint64_t>(tx.contract.caller.size());

            out.write(
                reinterpret_cast<const char*>(&callerSize),
                sizeof(callerSize)
            );

            if (callerSize > 0)
            {
                out.write(
                    reinterpret_cast<const char*>(
                        tx.contract.caller.data()
                    ),
                    static_cast<std::streamsize>(callerSize)
                );
            }

            // ========================================================
            // CONTRACT ADDRESS
            // ========================================================
            if (!WriteString(
                    out,
                    tx.contract.contractAddress))
                return false;

            // ========================================================
            // DEPLOYMENT NONCE
            // ========================================================
            out.write(
                reinterpret_cast<const char*>(
                    &tx.contract.deploymentNonce
                ),
                sizeof(tx.contract.deploymentNonce)
            );

            // ========================================================
            // BYTECODE
            // ========================================================
            uint64_t bytecodeSize =
                static_cast<uint64_t>(
                    tx.contract.bytecode.size()
                );

            out.write(
                reinterpret_cast<const char*>(&bytecodeSize),
                sizeof(bytecodeSize)
            );

            if (bytecodeSize > 0)
            {
                out.write(
                    reinterpret_cast<const char*>(
                        tx.contract.bytecode.data()
                    ),
                    static_cast<std::streamsize>(bytecodeSize)
                );
            }

            // ========================================================
            // CONTRACT INPUT
            // ========================================================
            uint64_t inputSize =
                static_cast<uint64_t>(
                    tx.contract.input.size()
                );

            out.write(
                reinterpret_cast<const char*>(&inputSize),
                sizeof(inputSize)
            );

            if (inputSize > 0)
            {
                out.write(
                    reinterpret_cast<const char*>(
                        tx.contract.input.data()
                    ),
                    static_cast<std::streamsize>(inputSize)
                );
            }

            // ========================================================
            // GAS LIMIT
            // ========================================================
            out.write(
                reinterpret_cast<const char*>(
                    &tx.contract.gasLimit
                ),
                sizeof(tx.contract.gasLimit)
            );

            if (!out)
                return false;

// Data khusus coinbase ikut disimpan.
        if (!WriteString(
                out,
                tx.coinbaseData))
            return false;

        // Inputs.

            uint64_t inputCount =
                tx.vin.size();

            out.write(
                reinterpret_cast<const char*>(
                    &inputCount
                ),
                sizeof(inputCount)
            );

            for (const auto& in : tx.vin)
            {
                if (!WriteString(
                        out,
                        in.previousTx))
                    return false;

                out.write(
                    reinterpret_cast<const char*>(
                        &in.outputIndex
                    ),
                    sizeof(in.outputIndex)
                );

                if (!WriteString(
                        out,
                        in.publicKey))
                    return false;

                if (!WriteString(
                        out,
                        in.signature))
                    return false;
            }

            // Outputs.
            uint64_t outputCount =
                tx.vout.size();

            out.write(
                reinterpret_cast<const char*>(
                    &outputCount
                ),
                sizeof(outputCount)
            );

            for (const auto& output : tx.vout)
            {
                if (!WriteString(
                        out,
                        output.address))
                    return false;

                out.write(
                    reinterpret_cast<const char*>(
                        &output.amount
                    ),
                    sizeof(output.amount)
                );

                out.write(
                    reinterpret_cast<const char*>(
                        &output.spent
                    ),
                    sizeof(output.spent)
                );
            }

            // Transaction-level Falcon signature.
            uint64_t signatureSize =
                tx.signature.size();

            out.write(
                reinterpret_cast<const char*>(
                    &signatureSize
                ),
                sizeof(signatureSize)
            );

            if (signatureSize > 0)
            {
                out.write(
                    reinterpret_cast<const char*>(
                        tx.signature.data()
                    ),
                    static_cast<std::streamsize>(
                        signatureSize
                    )
                );
            }

            // Transaction-level Falcon public key.
            uint64_t publicKeySize =
                tx.publicKey.size();

            out.write(
                reinterpret_cast<const char*>(
                    &publicKeySize
                ),
                sizeof(publicKeySize)
            );

            if (publicKeySize > 0)
            {
                out.write(
                    reinterpret_cast<const char*>(
                        tx.publicKey.data()
                    ),
                    static_cast<std::streamsize>(
                        publicKeySize
                    )
                );
            }
        }

        if (!out)
            return false;
    }

    return static_cast<bool>(out);
}





static bool StorageLoadFailExact(int line)
{
    std::cerr
        << "LOAD BLOCKCHAIN FAIL SOURCE_LINE="
        << line
        << std::endl;

    return false;
}

bool LoadBlockchain(
    BerylChain& chain,
    UTXOManager& utxoManager,
    const std::string& filename
)
{
    auto LoadFailStage = [&](const char* stage) -> bool {
        std::cerr
            << "LOAD FAIL STAGE="
            << stage
            << std::endl;
        return StorageLoadFailExact(__LINE__);
    };



    std::ifstream in(
        filename,
        std::ios::binary
    );

    if (!in)
        return LoadFailStage("OPEN_OR_READ");    // ============================
    // MAGIC
    // ============================

    const char expectedMagicV2[] =
        "BERYLCHAIN2";

    const char expectedMagicV3[] =
        "BERYLCHAIN3";

    char magic[11] = {};

    in.read(
        magic,
        sizeof(magic)
    );

    if (!in)
        return StorageLoadFailExact(__LINE__);

    const std::string fileMagic(
        magic,
        sizeof(magic)
    );

    const bool isV2 =
        fileMagic == expectedMagicV2;

    const bool isV3 =
        fileMagic == expectedMagicV3;

    if (!isV2 && !isV3)
    {
        std::cerr
            << "UNKNOWN BLOCKCHAIN MAGIC: "
            << fileMagic
            << std::endl;

        return StorageLoadFailExact(__LINE__);
    }

    // ============================
    // BLOCK COUNT
    // ============================

    uint64_t blockCount = 0;

    in.read(
        reinterpret_cast<char*>(&blockCount),
        sizeof(blockCount)
    );

    if (!in)
        return StorageLoadFailExact(__LINE__);    // Proteksi file corrupt / absurd.
    if (blockCount > 100000000ULL)
        return LoadFailStage("BLOCK_COUNT_LIMIT");
    // Kita load ke chain baru supaya
    // chain existing tidak rusak jika
    // file ternyata invalid.
    BerylChain loadedChain;

    for (uint64_t b = 0;
         b < blockCount;
         ++b)
    {
        BerylBlock block;

        // ============================
        // HEADER
        // ============================

        in.read(
            reinterpret_cast<char*>(
                &block.header.version
            ),
            sizeof(block.header.version)
        );

        if (!ReadString(
                in,
                block.header.previousHash))
            return StorageLoadFailExact(__LINE__);        if (!ReadString(
                in,
                block.header.merkleRoot))
            return StorageLoadFailExact(__LINE__);
        if (!ReadString(
                in,
                block.header.utxoRoot))
            return StorageLoadFailExact(__LINE__);
        if (isV3)
        {
            if (!ReadString(
                    in,
                    block.header.contractRoot))
                return StorageLoadFailExact(__LINE__);
        }
        else
        {
            // BERYLCHAIN2 tidak mempunyai contractRoot
            // pada header block.
            block.header.contractRoot.clear();
        }        in.read(
            reinterpret_cast<char*>(
                &block.header.timestamp
            ),
            sizeof(block.header.timestamp)
        );

        in.read(
            reinterpret_cast<char*>(
                &block.header.nonce
            ),
            sizeof(block.header.nonce)
        );

        in.read(
            reinterpret_cast<char*>(
                &block.header.difficulty
            ),
            sizeof(block.header.difficulty)
        );

        in.read(
            reinterpret_cast<char*>(
                &block.header.height
            ),
            sizeof(block.header.height)
        );

        // ============================
        // BLOCK
        // ============================

        in.read(
            reinterpret_cast<char*>(
                &block.reward
            ),
            sizeof(block.reward)
        );

        if (!ReadString(
                in,
                block.hash))
            return StorageLoadFailExact(__LINE__);        // ============================
        // TRANSACTIONS
        // ============================

        uint64_t txCount = 0;

        in.read(
            reinterpret_cast<char*>(&txCount),
            sizeof(txCount)
        );

        if (!in)
            return StorageLoadFailExact(__LINE__);        if (txCount > 1000000ULL)
            return StorageLoadFailExact(__LINE__);
        for (uint64_t t = 0;
             t < txCount;
             ++t)
        {
            BerylTransaction tx;

            if (!ReadString(
                    in,
                    tx.txid))
                return StorageLoadFailExact(__LINE__);
            
            // ========================================================
            // TRANSACTION FORMAT
            //
            // V2:
            //   txid
            //   coinbaseData
            //   inputs
            //   outputs
            //   Falcon signature
            //   Falcon public key
            //
            // V3:
            //   txid
            //   transaction type
            //   contract type
            //   contract caller
            //   contract address
            //   deployment nonce
            //   bytecode
            //   contract input
            //   gas limit
            //   coinbaseData
            //   inputs
            //   outputs
            //   Falcon signature
            //   Falcon public key
            // ========================================================

            if (isV3)
            {
                // ====================================================
                // TRANSACTION TYPE
                // ====================================================

                uint8_t txType = 0;

                in.read(
                    reinterpret_cast<char*>(&txType),
                    sizeof(txType)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                if (txType >
                    static_cast<uint8_t>(
                        TransactionType::CONTRACT_CALL
                    ))
                    return StorageLoadFailExact(__LINE__);

                tx.type =
                    static_cast<TransactionType>(txType);

                // ====================================================
                // CONTRACT TYPE
                // ====================================================

                uint8_t contractType = 0;

                in.read(
                    reinterpret_cast<char*>(&contractType),
                    sizeof(contractType)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                if (contractType >
                    static_cast<uint8_t>(
                        TransactionType::CONTRACT_CALL
                    ))
                    return StorageLoadFailExact(__LINE__);

                tx.contract.type =
                    static_cast<TransactionType>(contractType);

                // ====================================================
                // CONTRACT CALLER
                // ====================================================

                uint64_t callerSize = 0;

                in.read(
                    reinterpret_cast<char*>(&callerSize),
                    sizeof(callerSize)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                if (callerSize >
                    16ULL * 1024ULL * 1024ULL)
                    return StorageLoadFailExact(__LINE__);

                tx.contract.caller.resize(
                    static_cast<size_t>(callerSize)
                );

                if (callerSize > 0)
                {
                    in.read(
                        reinterpret_cast<char*>(
                            tx.contract.caller.data()
                        ),
                        static_cast<std::streamsize>(
                            callerSize
                        )
                    );

                    if (!in)
                        return StorageLoadFailExact(__LINE__);
                }

                // ====================================================
                // CONTRACT ADDRESS
                // ====================================================

                if (!ReadString(
                        in,
                        tx.contract.contractAddress))
                    return StorageLoadFailExact(__LINE__);

                // ====================================================
                // DEPLOYMENT NONCE
                // ====================================================

                in.read(
                    reinterpret_cast<char*>(
                        &tx.contract.deploymentNonce
                    ),
                    sizeof(tx.contract.deploymentNonce)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                // ====================================================
                // BYTECODE
                // ====================================================

                uint64_t bytecodeSize = 0;

                in.read(
                    reinterpret_cast<char*>(&bytecodeSize),
                    sizeof(bytecodeSize)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                if (bytecodeSize >
                    16ULL * 1024ULL * 1024ULL)
                    return StorageLoadFailExact(__LINE__);

                tx.contract.bytecode.resize(
                    static_cast<size_t>(bytecodeSize)
                );

                if (bytecodeSize > 0)
                {
                    in.read(
                        reinterpret_cast<char*>(
                            tx.contract.bytecode.data()
                        ),
                        static_cast<std::streamsize>(
                            bytecodeSize
                        )
                    );

                    if (!in)
                        return StorageLoadFailExact(__LINE__);
                }

                // ====================================================
                // CONTRACT INPUT
                // ====================================================

                uint64_t inputSize = 0;

                in.read(
                    reinterpret_cast<char*>(&inputSize),
                    sizeof(inputSize)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);

                if (inputSize >
                    16ULL * 1024ULL * 1024ULL)
                    return StorageLoadFailExact(__LINE__);

                tx.contract.input.resize(
                    static_cast<size_t>(inputSize)
                );

                if (inputSize > 0)
                {
                    in.read(
                        reinterpret_cast<char*>(
                            tx.contract.input.data()
                        ),
                        static_cast<std::streamsize>(
                            inputSize
                        )
                    );

                    if (!in)
                        return StorageLoadFailExact(__LINE__);
                }

                // ====================================================
                // GAS LIMIT
                // ====================================================

                in.read(
                    reinterpret_cast<char*>(
                        &tx.contract.gasLimit
                    ),
                    sizeof(tx.contract.gasLimit)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);
            }
            else
            {
                // ====================================================
                // V2 TIDAK MEMILIKI FIELD CONTRACT
                // ====================================================

                tx.contract.caller.clear();
                tx.contract.contractAddress.clear();
                tx.contract.deploymentNonce = 0;
                tx.contract.bytecode.clear();
                tx.contract.input.clear();
                tx.contract.gasLimit = 0;
            }

            // ========================================================
            // COINBASE DATA
            // ========================================================

            if (!ReadString(
                    in,
                    tx.coinbaseData))
                return StorageLoadFailExact(__LINE__);


            // Inputs.
            uint64_t inputCount = 0;

            in.read(
                reinterpret_cast<char*>(
                    &inputCount
                ),
                sizeof(inputCount)
            );

            if (!in)
                return StorageLoadFailExact(__LINE__);            if (inputCount > 1000000ULL)
                return StorageLoadFailExact(__LINE__);
            for (uint64_t i = 0;
                 i < inputCount;
                 ++i)
            {
                TxInput input;

                if (!ReadString(
                        in,
                        input.previousTx))
                    return StorageLoadFailExact(__LINE__);                in.read(
                    reinterpret_cast<char*>(
                        &input.outputIndex
                    ),
                    sizeof(input.outputIndex)
                );

                if (!ReadString(
                        in,
                        input.publicKey))
                    return StorageLoadFailExact(__LINE__);                if (!ReadString(
                        in,
                        input.signature))
                    return StorageLoadFailExact(__LINE__);
                tx.vin.push_back(input);
            }

            // Outputs.
            uint64_t outputCount = 0;

            in.read(
                reinterpret_cast<char*>(
                    &outputCount
                ),
                sizeof(outputCount)
            );

            if (!in)
                return StorageLoadFailExact(__LINE__);            if (outputCount > 1000000ULL)
                return StorageLoadFailExact(__LINE__);
            for (uint64_t o = 0;
                 o < outputCount;
                 ++o)
            {
                TxOutput output;

                if (!ReadString(
                        in,
                        output.address))
                    return StorageLoadFailExact(__LINE__);                in.read(
                    reinterpret_cast<char*>(
                        &output.amount
                    ),
                    sizeof(output.amount)
                );

                in.read(
                    reinterpret_cast<char*>(
                        &output.spent
                    ),
                    sizeof(output.spent)
                );

                if (!in)
                    return StorageLoadFailExact(__LINE__);                tx.vout.push_back(output);
            }

            // Falcon signature.
            uint64_t signatureSize = 0;

            in.read(
                reinterpret_cast<char*>(
                    &signatureSize
                ),
                sizeof(signatureSize)
            );

            if (!in)
                return StorageLoadFailExact(__LINE__);            if (signatureSize > 16ULL * 1024ULL * 1024ULL)
                return StorageLoadFailExact(__LINE__);
            tx.signature.resize(
                static_cast<size_t>(signatureSize)
            );

            if (signatureSize > 0)
            {
                in.read(
                    reinterpret_cast<char*>(
                        tx.signature.data()
                    ),
                    static_cast<std::streamsize>(
                        signatureSize
                    )
                );
            }

            // Falcon public key.
            uint64_t publicKeySize = 0;

            in.read(
                reinterpret_cast<char*>(
                    &publicKeySize
                ),
                sizeof(publicKeySize)
            );

            if (!in)
                return StorageLoadFailExact(__LINE__);            if (publicKeySize > 16ULL * 1024ULL * 1024ULL)
                return StorageLoadFailExact(__LINE__);
            tx.publicKey.resize(
                static_cast<size_t>(publicKeySize)
            );

            if (publicKeySize > 0)
            {
                in.read(
                    reinterpret_cast<char*>(
                        tx.publicKey.data()
                    ),
                    static_cast<std::streamsize>(
                        publicKeySize
                    )
                );
            }

            if (!in)
                return StorageLoadFailExact(__LINE__);            block.transactions.push_back(tx);
        }

        if (!in)
            return StorageLoadFailExact(__LINE__);
        // Block dimasukkan ke loaded chain.
        //
        // Kita tidak menggunakan AddBlock()
        // di sini karena AddBlock() melakukan
        // validasi dan ProcessBlock().
        //
        // Persistence layer tahap pertama hanya
        // memulihkan data chain. UTXO akan
        // direbuild setelah seluruh block selesai.
        if (!loadedChain.LoadBlock(block))
        return StorageLoadFailExact(__LINE__);
    }

    // ========================================================
    // Rebuild UTXO dari seluruh blockchain.
    // ========================================================

    UTXOManager rebuiltUTXO;

    rebuiltUTXO.Rebuild(
        loadedChain.GetBlocks()
    );

    // ========================================================
    // Rebuild Contract State dari seluruh blockchain.
    //
    // Contract state merupakan bagian dari consensus.
    // LoadBlock() hanya memulihkan block ke chain, sehingga
    // contract state harus direplay secara deterministic
    // dari genesis sampai block terakhir.
    // ========================================================

    beryl::contract::ContractStateManager rebuiltContractState;

    for (const BerylBlock& block : loadedChain.GetBlocks())
    {
        // Coinbase tidak mengubah contract state.
        for (size_t i = 1; i < block.transactions.size(); ++i)
        {
            const BerylTransaction& tx = block.transactions[i];

            if (!ApplyContractTransactionState(
                    rebuiltContractState,
                    tx,
                    block.header.height))
            {
                std::cerr
                    << "CONTRACT REPLAY FAILED"
                    << " HEIGHT=" << block.header.height
                    << " TXID=" << tx.txid
                    << " TYPE=" << static_cast<int>(tx.type)
                    << " CONTRACT_TYPE=" << static_cast<int>(tx.contract.type)
                    << "";

                return LoadFailStage("CONTRACT_REPLAY");
            }
        }

        // Contract root yang tersimpan pada block harus sama
        // dengan root hasil replay state sampai block tersebut.
        const std::string calculatedContractRoot =
            rebuiltContractState.CalculateRoot();

        if (calculatedContractRoot.empty())
        {
            std::cerr
                << "CONTRACT ROOT EMPTY"
                << " HEIGHT=" << block.header.height
                << "";

            return StorageLoadFailExact(__LINE__);
        }

                // ========================================================
        // CONTRACT ROOT VALIDATION
        //
        // BERYLCHAIN2:
        //   Tidak mempunyai contractRoot pada header.
        //   Root tidak boleh dibandingkan dengan string kosong.
        //
        // BERYLCHAIN3:
        //   contractRoot tersimpan dan wajib cocok dengan
        //   hasil deterministic replay.
        // ========================================================

        if (isV3)
        {
            // V3 lama dapat memiliki contractRoot kosong karena
            // block tersebut dibuat sebelum contractRoot diwajibkan.
            //
            // Block lama tetap valid karena root tidak dapat dibandingkan.
            // Untuk block V3 yang sudah mempunyai root, root wajib cocok.
            if (!block.header.contractRoot.empty() &&
                calculatedContractRoot != block.header.contractRoot)
            {
                std::cerr
                    << "CONTRACT ROOT MISMATCH"
                    << " HEIGHT=" << block.header.height
                    << " STORED=" << block.header.contractRoot
                    << " CALCULATED=" << calculatedContractRoot
                    << std::endl;

                return LoadFailStage("CONTRACT_ROOT");
            }
        }
        else
        {
            // V2 tidak menyimpan contractRoot.
            //
            // Root hasil replay tetap dihitung untuk memastikan
            // ContractState dapat direplay secara deterministic.
            //
            // Tidak ada perbandingan dengan block.header.contractRoot
            // karena field tersebut memang tidak ada pada format V2.
        }

    } // end for (const BerylBlock& block : loadedChain.GetBlocks())

    // ========================================================
    // Commit ContractState hasil rebuild.
    // ========================================================

    loadedChain.GetContractStateManager() = rebuiltContractState;

    // ========================================================
    // Commit hasil load setelah semua data terbaca.
    // ========================================================

    chain = loadedChain;

    chain.SetUTXOManager(
        &utxoManager
    );

    utxoManager =
        rebuiltUTXO;

    return true;
}
