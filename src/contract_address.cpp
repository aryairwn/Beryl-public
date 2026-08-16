#include "contract_address.h"

#include "crypto/blake3_wrapper.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace beryl::contract {

// ============================================================
// Bech32 V1
// ============================================================

namespace {

static constexpr char CHARSET[] =
    "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

uint32_t PolyMod(
    const std::vector<uint8_t>& values
)
{
    uint32_t chk = 1;

    for (uint8_t value : values)
    {
        const uint8_t top =
            static_cast<uint8_t>(chk >> 25);

        chk =
            ((chk & 0x1ffffff) << 5) ^ value;

        if (top & 1)  chk ^= 0x3b6a57b2;
        if (top & 2)  chk ^= 0x26508e6d;
        if (top & 4)  chk ^= 0x1ea119fa;
        if (top & 8)  chk ^= 0x3d4233dd;
        if (top & 16) chk ^= 0x2a1462b3;
    }

    return chk;
}

std::vector<uint8_t> HrpExpand(
    const std::string& hrp
)
{
    std::vector<uint8_t> result;

    for (char c : hrp)
        result.push_back(
            static_cast<uint8_t>(
                static_cast<unsigned char>(c) >> 5
            )
        );

    result.push_back(0);

    for (char c : hrp)
        result.push_back(
            static_cast<uint8_t>(
                static_cast<unsigned char>(c) & 31
            )
        );

    return result;
}

std::vector<uint8_t> CreateChecksum(
    const std::string& hrp,
    const std::vector<uint8_t>& data
)
{
    std::vector<uint8_t> values =
        HrpExpand(hrp);

    values.insert(
        values.end(),
        data.begin(),
        data.end()
    );

    values.insert(
        values.end(),
        6,
        0
    );

    const uint32_t mod =
        PolyMod(values) ^ 1;

    std::vector<uint8_t> checksum(6);

    for (size_t i = 0; i < 6; ++i)
    {
        checksum[i] =
            static_cast<uint8_t>(
                (mod >> (5 * (5 - i))) & 31
            );
    }

    return checksum;
}

std::vector<uint8_t> ConvertBits(
    const std::vector<uint8_t>& input
)
{
    std::vector<uint8_t> output;

    uint32_t accumulator = 0;
    int bits = 0;

    for (uint8_t value : input)
    {
        accumulator =
            (accumulator << 8) | value;

        bits += 8;

        while (bits >= 5)
        {
            bits -= 5;

            output.push_back(
                static_cast<uint8_t>(
                    (accumulator >> bits) & 31
                )
            );
        }
    }

    if (bits > 0)
    {
        output.push_back(
            static_cast<uint8_t>(
                (accumulator << (5 - bits)) & 31
            )
        );
    }

    return output;
}

} // anonymous namespace

// ============================================================
// Contract ID
// ============================================================

ContractId DeriveContractId(
    const std::vector<uint8_t>& deployerAccountId,
    uint64_t deploymentNonce
)
{
    std::vector<unsigned char> preimage;

    static constexpr uint8_t DOMAIN[] = {
        'B', 'V', 'M', '1'
    };

    preimage.insert(
        preimage.end(),
        DOMAIN,
        DOMAIN + sizeof(DOMAIN)
    );

    preimage.insert(
        preimage.end(),
        deployerAccountId.begin(),
        deployerAccountId.end()
    );

    // Canonical little-endian uint64 nonce.
    for (unsigned int i = 0; i < 8; ++i)
    {
        preimage.push_back(
            static_cast<uint8_t>(
                (deploymentNonce >> (8 * i)) & 0xff
            )
        );
    }

    const std::vector<unsigned char> hash =
        Blake3Hash(
            preimage,
            CONTRACT_ID_SIZE
        );

    ContractId id{};

    if (hash.size() != CONTRACT_ID_SIZE)
        return id;

    std::copy(
        hash.begin(),
        hash.end(),
        id.begin()
    );

    return id;
}

// ============================================================
// Bech32 encoding
// ============================================================

std::string EncodeContractAddress(
    const ContractId& contractId
)
{
    static constexpr char HRP[] = "bvm";

    std::vector<uint8_t> bytes(
        contractId.begin(),
        contractId.end()
    );

    const std::vector<uint8_t> data =
        ConvertBits(bytes);

    const std::vector<uint8_t> checksum =
        CreateChecksum(
            HRP,
            data
        );

    std::string result = "bvm1";

    for (uint8_t value : data)
        result += CHARSET[value];

    for (uint8_t value : checksum)
        result += CHARSET[value];

    return result;
}

// ============================================================
// Bech32 decoding
// ============================================================

namespace {

bool ConvertBitsDecode(
    const std::vector<uint8_t>& input,
    std::vector<uint8_t>& output
)
{
    output.clear();

    uint32_t accumulator = 0;
    int bits = 0;

    for (uint8_t value : input)
    {
        if (value > 31)
            return false;

        accumulator = (accumulator << 5) | value;
        bits += 5;

        while (bits >= 8)
        {
            bits -= 8;

            output.push_back(
                static_cast<uint8_t>(
                    (accumulator >> bits) & 0xff
                )
            );
        }
    }

    // Bech32 conversion must not contain non-zero padding.
    if (bits >= 5)
        return false;

    if (bits > 0 &&
        ((accumulator << (8 - bits)) & 0xff) != 0)
    {
        return false;
    }

    return true;
}

int CharsetValue(char c)
{
    for (int i = 0; i < 32; ++i)
    {
        if (CHARSET[i] == c)
            return i;
    }

    return -1;
}

bool VerifyChecksum(
    const std::string& hrp,
    const std::vector<uint8_t>& data
)
{
    std::vector<uint8_t> values = HrpExpand(hrp);

    values.insert(
        values.end(),
        data.begin(),
        data.end()
    );

    return PolyMod(values) == 1;
}

} // anonymous namespace

bool DecodeContractAddress(
    const std::string& address,
    ContractId& contractId
)
{
    contractId.fill(0);

    // Canonical contract address:
    //     bvm1...
    static constexpr char HRP[] = "bvm";

    if (address.size() < 8)
        return false;

    // Bech32 V1 here is lowercase-only.
    for (char c : address)
    {
        if (c < '!' || c > '~')
            return false;

        if (c >= 'A' && c <= 'Z')
            return false;
    }

    const std::string prefix = "bvm1";

    if (address.compare(0, prefix.size(), prefix) != 0)
        return false;

    // Separator is fixed at position 3 for bvm1...
    const size_t separator = address.find('1');

    if (separator != 3)
        return false;

    const size_t dataStart = separator + 1;

    // 32-byte contract ID -> 52 x 5-bit values,
    // plus 6 checksum values.
    if (address.size() != dataStart + 52 + 6)
        return false;

    std::vector<uint8_t> data;

    data.reserve(address.size() - dataStart);

    for (size_t i = dataStart; i < address.size(); ++i)
    {
        const int value = CharsetValue(address[i]);

        if (value < 0)
            return false;

        data.push_back(
            static_cast<uint8_t>(value)
        );
    }

    if (data.size() < 6)
        return false;

    if (!VerifyChecksum(HRP, data))
        return false;

    // Remove six checksum symbols.
    data.resize(data.size() - 6);

    std::vector<uint8_t> decoded;

    if (!ConvertBitsDecode(data, decoded))
        return false;

    if (decoded.size() != CONTRACT_ID_SIZE)
        return false;

    std::copy(
        decoded.begin(),
        decoded.end(),
        contractId.begin()
    );

    // Canonical round-trip check.
    // This prevents accepting alternate encodings that decode
    // to the same 32-byte identifier.
    if (EncodeContractAddress(contractId) != address)
    {
        contractId.fill(0);
        return false;
    }

    return true;
}

// ============================================================
// Derive + encode
// ============================================================

std::string DeriveContractAddress(
    const std::vector<uint8_t>& deployerAccountId,
    uint64_t deploymentNonce
)
{
    const ContractId id =
        DeriveContractId(
            deployerAccountId,
            deploymentNonce
        );

    return EncodeContractAddress(id);
}

} // namespace beryl::contract
