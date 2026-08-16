// © Arya Irawan — 10 August 2026

#include "transaction.h"
#include "consensus.h"
#include "crypto/blake3_wrapper.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <utility>

// ============================================================
// HEX
// ============================================================

static std::string ToHex(
    const std::vector<unsigned char>& data
)
{
    std::stringstream ss;

    for (unsigned char b : data)
    {
        ss << std::hex
           << std::setw(2)
           << std::setfill('0')
           << static_cast<int>(b);
    }

    return ss.str();
}

// ============================================================
// SERIALIZE TRANSACTION
// ============================================================

std::string SerializeBerylTransaction(
    const BerylTransaction& tx
)
{
    std::string out;

    auto WriteU32 = [](
        std::string& out,
        uint32_t value
    )
    {
        for (int i = 0; i < 4; ++i)
        {
            out.push_back(
                static_cast<char>(
                    (value >> (8 * i)) & 0xff
                )
            );
        }
    };

    auto WriteU64 = [](
        std::string& out,
        uint64_t value
    )
    {
        for (int i = 0; i < 8; ++i)
        {
            out.push_back(
                static_cast<char>(
                    (value >> (8 * i)) & 0xff
                )
            );
        }
    };

    auto WriteString = [&](
        std::string& out,
        const std::string& value
    )
    {
        WriteU32(
            out,
            static_cast<uint32_t>(value.size())
        );

        out.append(value);
    };

    auto WriteBytes = [&](
        std::string& out,
        const std::vector<unsigned char>& value
    )
    {
        WriteU32(
            out,
            static_cast<uint32_t>(value.size())
        );

        out.append(
            reinterpret_cast<const char*>(
                value.data()
            ),
            value.size()
        );
    };

    auto WriteStringBytes = [&](
        std::string& out,
        const std::string& value
    )
    {
        WriteU32(
            out,
            static_cast<uint32_t>(value.size())
        );

        out.append(value);
    };

    WriteString(
        out,
        tx.txid
    );

    // Native transaction type.
    out.push_back(
        static_cast<char>(
            static_cast<uint8_t>(tx.type)
        )
    );

    // Contract payload.
    out.push_back(
        static_cast<char>(
            static_cast<uint8_t>(tx.contract.type)
        )
    );

    WriteBytes(
        out,
        tx.contract.caller
    );

    WriteString(
        out,
        tx.contract.contractAddress
    );

    WriteU64(
        out,
        tx.contract.deploymentNonce
    );

    WriteBytes(
        out,
        tx.contract.bytecode
    );

    WriteBytes(
        out,
        tx.contract.input
    );

    WriteU64(
        out,
        tx.contract.gasLimit
    );

    WriteString(
        out,
        tx.coinbaseData
    );

    WriteU32(
        out,
        static_cast<uint32_t>(tx.vin.size())
    );

    for (const auto& in : tx.vin)
    {
        WriteString(
            out,
            in.previousTx
        );

        WriteU32(
            out,
            in.outputIndex
        );

        WriteStringBytes(
            out,
            in.signature
        );

        WriteStringBytes(
            out,
            in.publicKey
        );
    }

    WriteU32(
        out,
        static_cast<uint32_t>(tx.vout.size())
    );

    for (const auto& output : tx.vout)
    {
        WriteString(
            out,
            output.address
        );

        WriteU64(
            out,
            output.amount
        );

        out.push_back(
            output.spent ? '\1' : '\0'
        );
    }

    WriteBytes(
        out,
        tx.signature
    );

    WriteBytes(
        out,
        tx.publicKey
    );

    return out;
}

// ============================================================
// PUBLIC TRANSACTION DESERIALIZATION
//
// Format sama dengan wire transaction P2P:
//
// txid
// coinbaseData
// vin[]
// vout[]
// transaction signature
// transaction public key
// ============================================================

static bool ReadU32Wallet(
    const std::string& data,
    size_t& pos,
    uint32_t& value
)
{
    if (pos + sizeof(uint32_t) > data.size())
        return false;

    std::memcpy(
        &value,
        data.data() + pos,
        sizeof(uint32_t)
    );

    pos += sizeof(uint32_t);
    return true;
}

static bool ReadU64Wallet(
    const std::string& data,
    size_t& pos,
    uint64_t& value
)
{
    if (pos + sizeof(uint64_t) > data.size())
        return false;

    std::memcpy(
        &value,
        data.data() + pos,
        sizeof(uint64_t)
    );

    pos += sizeof(uint64_t);
    return true;
}

static bool ReadStringWallet(
    const std::string& data,
    size_t& pos,
    std::string& value
)
{
    uint32_t size = 0;

    if (!ReadU32Wallet(data, pos, size))
        return false;

    if (size > data.size() - pos)
        return false;

    value.assign(
        data.data() + pos,
        size
    );

    pos += size;
    return true;
}

static bool ReadBytesWallet(
    const std::string& data,
    size_t& pos,
    std::vector<unsigned char>& value
)
{
    uint32_t size = 0;

    if (!ReadU32Wallet(data, pos, size))
        return false;

    if (size > data.size() - pos)
        return false;

    value.assign(
        reinterpret_cast<const unsigned char*>(
            data.data() + pos
        ),
        reinterpret_cast<const unsigned char*>(
            data.data() + pos + size
        )
    );

    pos += size;
    return true;
}

static bool ReadBytesWallet(
    const std::string& data,
    size_t& pos,
    std::string& value
)
{
    uint32_t size = 0;

    if (!ReadU32Wallet(data, pos, size))
        return false;

    if (size > data.size() - pos)
        return false;

    value.assign(
        data.data() + pos,
        size
    );

    pos += size;
    return true;
}

bool DeserializeBerylTransaction(
    const std::string& data,
    BerylTransaction& tx
)
{
    tx = BerylTransaction();

    size_t pos = 0;

    if (!ReadStringWallet(
            data,
            pos,
            tx.txid))
    {
        return false;
    }

    // Native transaction type.
    if (pos >= data.size())
        return false;

    const uint8_t rawType =
        static_cast<uint8_t>(
            static_cast<unsigned char>(data[pos++])
        );

    if (rawType >
        static_cast<uint8_t>(
            TransactionType::CONTRACT_CALL))
    {
        return false;
    }

    tx.type =
        static_cast<TransactionType>(rawType);

    // Contract payload type.
    if (pos >= data.size())
        return false;

    const uint8_t rawContractType =
        static_cast<uint8_t>(
            static_cast<unsigned char>(data[pos++])
        );

    if (rawContractType >
        static_cast<uint8_t>(
            TransactionType::CONTRACT_CALL))
    {
        return false;
    }

    tx.contract.type =
        static_cast<TransactionType>(
            rawContractType
        );

    if (!ReadBytesWallet(
            data,
            pos,
            tx.contract.caller))
    {
        return false;
    }

    if (!ReadStringWallet(
            data,
            pos,
            tx.contract.contractAddress))
    {
        return false;
    }

    if (!ReadU64Wallet(
            data,
            pos,
            tx.contract.deploymentNonce))
    {
        return false;
    }

    if (!ReadBytesWallet(
            data,
            pos,
            tx.contract.bytecode))
    {
        return false;
    }

    if (!ReadBytesWallet(
            data,
            pos,
            tx.contract.input))
    {
        return false;
    }

    if (!ReadU64Wallet(
            data,
            pos,
            tx.contract.gasLimit))
    {
        return false;
    }

    if (!ReadStringWallet(
            data,
            pos,
            tx.coinbaseData))
    {
        return false;
    }

    uint32_t vinCount = 0;

    if (!ReadU32Wallet(
            data,
            pos,
            vinCount))
    {
        return false;
    }

    if (vinCount > 100000)
        return false;

    for (uint32_t i = 0;
         i < vinCount;
         ++i)
    {
        TxInput input;

        if (!ReadStringWallet(
                data,
                pos,
                input.previousTx))
        {
            return false;
        }

        if (!ReadU32Wallet(
                data,
                pos,
                input.outputIndex))
        {
            return false;
        }

        if (!ReadBytesWallet(
                data,
                pos,
                input.signature))
        {
            return false;
        }

        if (!ReadBytesWallet(
                data,
                pos,
                input.publicKey))
        {
            return false;
        }

        tx.vin.push_back(
            std::move(input)
        );
    }

    uint32_t voutCount = 0;

    if (!ReadU32Wallet(
            data,
            pos,
            voutCount))
    {
        return false;
    }

    if (voutCount > 100000)
        return false;

    for (uint32_t i = 0;
         i < voutCount;
         ++i)
    {
        TxOutput output;

        if (!ReadStringWallet(
                data,
                pos,
                output.address))
        {
            return false;
        }

        if (!ReadU64Wallet(
                data,
                pos,
                output.amount))
        {
            return false;
        }

        if (pos >= data.size())
            return false;

        output.spent =
            data[pos++] != 0;

        tx.vout.push_back(
            std::move(output)
        );
    }

    if (!ReadBytesWallet(
            data,
            pos,
            tx.signature))
    {
        return false;
    }

    if (!ReadBytesWallet(
            data,
            pos,
            tx.publicKey))
    {
        return false;
    }

    // Tidak boleh ada data tambahan.
    if (pos != data.size())
        return false;

    return true;
}


// ============================================================
// TXID = BLAKE3-160
// ============================================================

std::string CalculateTxID(
    const BerylTransaction& tx
)
{
    /*
     * TXID harus deterministik dan tidak boleh
     * menghitung dirinya sendiri.
     *
     * TXID juga tidak bergantung pada:
     * - txid
     * - transaction-level signature
     * - transaction-level publicKey
     *
     * Signature Falcon diverifikasi terpisah.
     */
    BerylTransaction canonical = tx;

    canonical.txid.clear();
    canonical.signature.clear();
    canonical.publicKey.clear();

    const std::string serialized =
        SerializeBerylTransaction(canonical);

    std::vector<unsigned char> data(
        serialized.begin(),
        serialized.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(data, 20);

    return ToHex(hash);
}

// ============================================================
// SIGNING HASH
// ============================================================

std::string CalculateSigningHash(
    const BerylTransaction& tx
)
{
    /*
     * CANONICAL SIGNING PAYLOAD V1
     *
     * Yang DITANDATANGANI:
     *   - transaction type
     *   - contract type
     *   - contract caller
     *   - contract address
     *   - deployment nonce
     *   - contract bytecode
     *   - contract input
     *   - gas limit
     *   - coinbase data
     *   - seluruh input reference
     *   - input public key
     *   - seluruh output
     *
     * Yang TIDAK DITANDATANGANI:
     *   - txid
     *   - transaction-level signature
     *   - transaction-level publicKey
     *   - input signature
     *
     * Input signature tidak ikut karena signature Falcon
     * transaction-level yang diverifikasi di V1.
     *
     * Semua integer menggunakan little-endian.
     * Semua byte/string diberi panjang eksplisit.
     */

    std::string data;
    data.reserve(1024);

    auto WriteU8 = [&](
        uint8_t value
    )
    {
        data.push_back(
            static_cast<char>(value)
        );
    };

    auto WriteU32 = [&](
        uint32_t value
    )
    {
        for (int i = 0; i < 4; ++i)
        {
            data.push_back(
                static_cast<char>(
                    (value >> (8 * i)) & 0xff
                )
            );
        }
    };

    auto WriteU64 = [&](
        uint64_t value
    )
    {
        for (int i = 0; i < 8; ++i)
        {
            data.push_back(
                static_cast<char>(
                    (value >> (8 * i)) & 0xff
                )
            );
        }
    };

    auto WriteBytes = [&](
        const std::vector<unsigned char>& value
    )
    {
        if (value.size() > UINT32_MAX)
            return false;

        WriteU32(
            static_cast<uint32_t>(value.size())
        );

        if (!value.empty())
        {
            data.append(
                reinterpret_cast<const char*>(
                    value.data()
                ),
                value.size()
            );
        }

        return true;
    };

    auto WriteString = [&](
        const std::string& value
    )
    {
        if (value.size() > UINT32_MAX)
            return false;

        WriteU32(
            static_cast<uint32_t>(value.size())
        );

        data.append(value);

        return true;
    };

    // --------------------------------------------------------
    // Domain separator / signing format version.
    // --------------------------------------------------------

    static constexpr char DOMAIN[] =
        "BERYL-TX-SIGN-V1";

    data.append(
        DOMAIN,
        sizeof(DOMAIN) - 1
    );

    // --------------------------------------------------------
    // Native transaction type.
    // --------------------------------------------------------

    WriteU8(
        static_cast<uint8_t>(tx.type)
    );

    // --------------------------------------------------------
    // Contract payload.
    // --------------------------------------------------------

    WriteU8(
        static_cast<uint8_t>(tx.contract.type)
    );

    if (!WriteBytes(tx.contract.caller))
        return {};

    if (!WriteString(tx.contract.contractAddress))
        return {};

    WriteU64(
        tx.contract.deploymentNonce
    );

    if (!WriteBytes(tx.contract.bytecode))
        return {};

    if (!WriteBytes(tx.contract.input))
        return {};

    WriteU64(
        tx.contract.gasLimit
    );

    // --------------------------------------------------------
    // Coinbase data.
    // --------------------------------------------------------

    if (!WriteString(tx.coinbaseData))
        return {};

    // --------------------------------------------------------
    // Inputs.
    //
    // Input signature intentionally excluded.
    // Input public key IS included.
    // --------------------------------------------------------

    if (tx.vin.size() > UINT32_MAX)
        return {};

    WriteU32(
        static_cast<uint32_t>(tx.vin.size())
    );

    for (const auto& in : tx.vin)
    {
        if (!WriteString(in.previousTx))
            return {};

        WriteU32(
            in.outputIndex
        );

        if (!WriteString(in.publicKey))
            return {};
    }

    // --------------------------------------------------------
    // Outputs.
    // --------------------------------------------------------

    if (tx.vout.size() > UINT32_MAX)
        return {};

    WriteU32(
        static_cast<uint32_t>(tx.vout.size())
    );

    for (const auto& out : tx.vout)
    {
        if (!WriteString(out.address))
            return {};

        WriteU64(
            out.amount
        );

        WriteU8(
            out.spent ? 1 : 0
        );
    }

    // --------------------------------------------------------
    // BLAKE3-160
    // --------------------------------------------------------

    const std::vector<unsigned char> bytes(
        data.begin(),
        data.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(bytes, 20);

    return ToHex(hash);
}

// ============================================================
// GLOBAL SIMPLE UTXO
// ============================================================

static UTXOSet g_utxoSet;

// ============================================================
// ADD UTXO
// ============================================================

void AddUTXO(
    const std::string& txid,
    uint32_t index,
    const TxOutput& out
)
{
    g_utxoSet.Add(
        txid,
        index,
        out.address,
        out.amount
    );
}

// ============================================================
// SPEND UTXO
// ============================================================

bool SpendUTXO(
    const std::string& txid,
    uint32_t index
)
{
    return g_utxoSet.Spend(
        txid,
        index
    );
}

// ============================================================
// GET UTXO
// ============================================================

bool GetUTXO(
    const std::string& txid,
    uint32_t index,
    TxOutput& out
)
{
    std::vector<UTXO> all =
        g_utxoSet.GetByAddress("");

    for (const auto& u : all)
    {
        if (u.txid == txid &&
            u.index == index)
        {
            out.address = u.address;
            out.amount = u.amount;
            out.spent = false;

            return true;
        }
    }

    return false;
}

// ============================================================
// CREATE TRANSACTION
// ============================================================

bool CreateTransaction(
    UTXOSet& utxos,
    const std::string& from,
    const std::string& to,
    uint64_t amount,
    BerylTransaction& tx,
    int currentHeight,
    uint64_t fee,
    const std::vector<std::string>& reservedInputs
)
{
    if (amount == 0)
        return false;

    // Fee harus memenuhi minimum consensus.
    if (fee < MIN_TRANSACTION_FEE)
        return false;

    // Amount + fee tidak boleh overflow.
    if (amount > UINT64_MAX - fee)
        return false;

    const uint64_t required =
        amount + fee;

    tx.txid.clear();
    tx.vin.clear();
    tx.vout.clear();
    tx.signature.clear();
    tx.publicKey.clear();

    uint64_t total = 0;

    std::vector<UTXO> available =
        utxos.GetByAddress(from);

    // Pilih UTXO sampai cukup.
    for (const auto& u : available)
    {
        // --------------------------------------------------------
        // MEMPOOL-AWARE UTXO SELECTION
        // --------------------------------------------------------
        // UTXO yang sudah dipakai transaksi di mempool dianggap
        // sementara RESERVED dan tidak boleh dipakai transaksi baru.
        // Format reserved input:
        //   txid:index
        // --------------------------------------------------------

        const std::string inputId =
            u.txid + ":" + std::to_string(u.index);

        bool reserved = false;

        for (const auto& reservedId : reservedInputs)
        {
            if (reservedId == inputId)
            {
                reserved = true;
                break;
            }
        }

        if (reserved)
            continue;

        // Coinbase harus matang minimal 10 blok sebelum dapat dibelanjakan.
        if (u.coinbase && currentHeight >= 0)
        {
            if (currentHeight < u.height)
                continue;

            if (static_cast<uint32_t>(currentHeight - u.height)
                < BerylConsensus::COINBASE_MATURITY)
                continue;
        }

        TxInput input;

        input.previousTx =
            u.txid;

        input.outputIndex =
            u.index;

        input.publicKey.clear();
        input.signature.clear();

        tx.vin.push_back(input);

        // Cegah overflow total input.
        if (UINT64_MAX - total < u.amount)
            return false;

        total += u.amount;

        // Harus cukup untuk amount + minimum fee.
        if (total >= required)
            break;
    }

    // Saldo harus cukup untuk amount + minimum fee.
    if (total < required)
    {
        tx.vin.clear();
        return false;
    }

    // Output penerima.
    TxOutput recipient;

    recipient.address = to;
    recipient.amount = amount;
    recipient.spent = false;

    tx.vout.push_back(recipient);

    // Kembalian setelah fee aktual dipotong.
    uint64_t changeAmount =
        total - amount - fee;

    if (changeAmount > 0)
    {
        TxOutput change;

        change.address = from;
        change.amount = changeAmount;
        change.spent = false;

        tx.vout.push_back(change);
    }

    tx.txid =
        CalculateTxID(tx);

    return true;
}

// ============================================================
// SIGN TRANSACTION
// ============================================================

bool SignTransaction(
    BerylTransaction& tx,
    FalconKey& key
)
{
    /*
     * Signing hash TIDAK mengandung:
     * - txid
     * - signature
     * - transaction-level publicKey
     *
     * Jadi signature tidak menandatangani dirinya sendiri.
     */
    std::string message =
        CalculateSigningHash(tx);

    std::string signature =
        key.Sign(message);

    if (signature.empty())
        return false;

    tx.signature.assign(
        signature.begin(),
        signature.end()
    );

    tx.publicKey =
        key.GetPublicKeyRaw();

    /*
     * TXID dihitung setelah seluruh data transaksi
     * tersedia. CalculateTxID() sendiri tidak
     * menggunakan signature/publicKey.
     */
    tx.txid =
        CalculateTxID(tx);

    return true;
}


bool VerifyTransactionSignature(
    const BerylTransaction& tx
)
{
    if (tx.signature.empty())
        return false;

    if (tx.publicKey.empty())
        return false;

    FalconKey key;

    std::vector<unsigned char> emptyPriv;

    if (!key.SetKeys(
            tx.publicKey,
            emptyPriv))
    {
        return false;
    }

    /*
     * HARUS sama persis dengan message
     * yang digunakan saat signing.
     */
    std::string message =
        CalculateSigningHash(tx);

    /*
     * tx.signature disimpan sebagai HEX string
     * karena FalconKey::Sign() mengembalikan HEX.
     *
     * Jangan di-HEX-kan lagi.
     */
    std::string signatureHex(
        tx.signature.begin(),
        tx.signature.end()
    );

    return key.Verify(
        message,
        signatureHex
    );
}


// ============================================================
// ADDRESS OWNERSHIP CHECK
// ============================================================
// Public key Falcon yang menandatangani transaksi harus
// menghasilkan address Beryl yang sama dengan pemilik UTXO.
// ============================================================

static bool PublicKeyMatchesAddress(
    const std::vector<unsigned char>& publicKey,
    const std::string& expectedAddress
)
{
    if (publicKey.empty())
        return false;

    FalconKey key;
    std::vector<unsigned char> emptyPriv;

    if (!key.SetKeys(publicKey, emptyPriv))
        return false;

    const std::string pubHex =
        key.GetPublicKeyHex();

    std::vector<unsigned char> pub(
        pubHex.begin(),
        pubHex.end()
    );

    const std::vector<unsigned char> hash =
        Blake3Hash(pub, 20);

    static const char hex[] =
        "0123456789abcdef";

    std::string derivedAddress = "ber";

    for (unsigned char b : hash)
    {
        derivedAddress +=
            hex[(b >> 4) & 0x0F];

        derivedAddress +=
            hex[b & 0x0F];
    }

    return derivedAddress == expectedAddress;
}

// ============================================================
// TRANSACTION VALIDATION
// ============================================================

bool ValidateTransaction(
    const UTXOSet& utxos,
    const BerylTransaction& tx,
    int currentHeight
)
{
    // --------------------------------------------------------
    // 0. Native transaction type dan contract payload type
    //    HARUS identik.
    //
    // Keduanya diserialisasi dan ikut signing hash/TXID.
    // Membiarkan keduanya berbeda dapat menghasilkan
    // representasi transaksi yang ambigu.
    // --------------------------------------------------------
    if (tx.type != tx.contract.type)
        return false;

    // --------------------------------------------------------
    // 1. Input dan output wajib tersedia.
    // --------------------------------------------------------

    if (tx.vin.empty())
        return false;

    if (tx.vout.empty())
        return false;

    // --------------------------------------------------------
    // 2. Signature Falcon wajib valid.
    // --------------------------------------------------------

    if (!VerifyTransactionSignature(tx))
        return false;

    // --------------------------------------------------------
    // 3. Ambil seluruh UTXO.
    //
    // GetByAddress("") pada implementasi UTXOSet saat ini
    // mengembalikan seluruh UTXO.
    // --------------------------------------------------------

    std::vector<UTXO> allUtxos =
        utxos.GetByAddress("");

    // --------------------------------------------------------
    // 4. Hitung total input dan pastikan setiap input
    //    menunjuk UTXO yang benar-benar tersedia.
    // --------------------------------------------------------

    uint64_t totalInput = 0;

    // Simpan input yang sudah dipakai untuk mendeteksi
    // double-spend di dalam satu transaksi.
    std::vector<std::string> spentInputs;

    for (const auto& in : tx.vin)
    {
        bool found = false;

        std::string inputId =
            in.previousTx + ":" +
            std::to_string(in.outputIndex);

        // UTXO yang sama tidak boleh muncul dua kali.
        for (const auto& used : spentInputs)
        {
            if (used == inputId)
                return false;
        }

        for (const auto& u : allUtxos)
        {
            if (u.txid == in.previousTx &&
                u.index == in.outputIndex)
            {
                found = true;

                  // Coinbase harus matang minimal 10 blok.
                  if (u.coinbase)
                  {
                      if (currentHeight < u.height)
                          return false;

                      if (static_cast<uint32_t>(currentHeight - u.height)
                          < BerylConsensus::COINBASE_MATURITY)
                          return false;
                  }


                // --------------------------------------------------------
                // ADDRESS OWNERSHIP
                // --------------------------------------------------------
                // Public key penanda tangan harus menghasilkan
                // address yang sama dengan pemilik UTXO.
                // --------------------------------------------------------

                if (!PublicKeyMatchesAddress(
                        tx.publicKey,
                        u.address
                    ))
                {
                    return false;
                }

                // Cegah overflow uint64_t.
                if (UINT64_MAX - totalInput < u.amount)
                    return false;

                totalInput += u.amount;

                break;
            }
        }

        // Input menunjuk UTXO yang tidak tersedia.
        if (!found)
            return false;

        spentInputs.push_back(inputId);
    }

    // --------------------------------------------------------
    // 5. Semua output harus mempunyai nilai > 0.
    // --------------------------------------------------------

    uint64_t totalOutput = 0;

    for (const auto& out : tx.vout)
    {
        if (out.amount == 0)
            return false;

        // Cegah overflow.
        if (UINT64_MAX - totalOutput < out.amount)
            return false;

        totalOutput += out.amount;
    }

    // --------------------------------------------------------
    // 6. Input harus mencukupi output.
    // --------------------------------------------------------

    if (totalInput < totalOutput)
        return false;

    // --------------------------------------------------------
    // 7. Validasi minimum transaction fee.
    //
    // Fee = total input - total output.
    // Minimum = 1 unit = 0.00000001 BER.
    // --------------------------------------------------------

    uint64_t fee =
        totalInput - totalOutput;

    if (fee < MIN_TRANSACTION_FEE)
        return false;

    // --------------------------------------------------------
    // 8. Semua pemeriksaan berhasil.
    // --------------------------------------------------------

    return true;
}

// ============================================================
// CALCULATE TRANSACTION FEE
// ============================================================

uint64_t CalculateTransactionFee(
    const UTXOSet& utxos,
    const BerylTransaction& tx
)
{
    // Transaksi invalid tidak mempunyai fee yang valid.
    if (!ValidateTransaction(utxos, tx))
        return 0;

    std::vector<UTXO> allUtxos =
        utxos.GetByAddress("");

    uint64_t totalInput = 0;
    uint64_t totalOutput = 0;

    // --------------------------------------------------------
    // Hitung total input.
    // --------------------------------------------------------

    for (const auto& in : tx.vin)
    {
        for (const auto& u : allUtxos)
        {
            if (u.txid == in.previousTx &&
                u.index == in.outputIndex)
            {
                // Cegah overflow.
                if (UINT64_MAX - totalInput < u.amount)
                    return 0;

                totalInput += u.amount;
                break;
            }
        }
    }

    // --------------------------------------------------------
    // Hitung total output.
    // --------------------------------------------------------

    for (const auto& out : tx.vout)
    {
        if (UINT64_MAX - totalOutput < out.amount)
            return 0;

        totalOutput += out.amount;
    }

    // --------------------------------------------------------
    // Karena ValidateTransaction() sudah memastikan
    // totalInput >= totalOutput, pengurangan aman.
    // --------------------------------------------------------

    return totalInput - totalOutput;
}


// ============================================================
// APPLY TRANSACTION
// ============================================================

bool ApplyTransaction(
    UTXOSet& utxos,
    const BerylTransaction& tx,
    int currentHeight
)
{
    // --------------------------------------------------------
    // 1. Validasi transaksi sebelum mengubah UTXO.
    // --------------------------------------------------------

    if (!ValidateTransaction(
            utxos,
            tx,
            currentHeight
        ))
        return false;

    // --------------------------------------------------------
    // 2. Pastikan seluruh input masih tersedia.
    //
    // ValidateTransaction() sudah melakukan pemeriksaan ini,
    // tetapi kita ulangi sebelum mutasi state agar alurnya jelas.
    // --------------------------------------------------------

    std::vector<UTXO> allUtxos =
        utxos.GetByAddress("");

    for (const auto& in : tx.vin)
    {
        bool found = false;

        for (const auto& u : allUtxos)
        {
            if (u.txid == in.previousTx &&
                u.index == in.outputIndex)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    // --------------------------------------------------------
    // 3. Spend seluruh input.
    //
    // Semua input sudah dipastikan tersedia di atas.
    // --------------------------------------------------------

    for (const auto& in : tx.vin)
    {
        if (!utxos.Spend(
                in.previousTx,
                in.outputIndex))
        {
            // Secara normal tidak boleh terjadi karena
            // semua input sudah diperiksa sebelumnya.
            return false;
        }
    }

    // --------------------------------------------------------
    // 4. Tambahkan seluruh output sebagai UTXO baru.
    // --------------------------------------------------------

    for (uint32_t i = 0;
         i < tx.vout.size();
         ++i)
    {
        UTXO out;

        out.txid = tx.txid;
        out.index = i;
        out.address = tx.vout[i].address;
        out.amount = tx.vout[i].amount;

        // Output transaksi biasa dibuat pada height block saat ini.
        out.height = currentHeight;
        out.coinbase = false;

        utxos.Add(out);
    }

    return true;
}






// ============================================================
// CALCULATE BLOCK FEES
// ============================================================

uint64_t CalculateBlockFees(
    const UTXOSet& utxos,
    const std::vector<BerylTransaction>& transactions
)
{
    uint64_t totalFees = 0;

    for (const auto& tx : transactions)
    {
        // Coinbase tidak memiliki input.
        if (tx.vin.empty())
            continue;

        uint64_t fee =
            CalculateTransactionFee(utxos, tx);

        // Transaksi non-coinbase harus mempunyai
        // minimal fee Beryl.
        if (fee < MIN_TRANSACTION_FEE)
            return 0;

        // Cegah overflow.
        if (UINT64_MAX - totalFees < fee)
            return 0;

        totalFees += fee;
    }

    return totalFees;
}
