#include "falcon.h"
#include "falcon512/falcon512.h"

#include <sstream>
#include <iomanip>

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
// HEX -> RAW
// ============================================================

static bool FromHex(
    const std::string& hex,
    std::vector<unsigned char>& data
)
{
    if (hex.size() % 2 != 0)
        return false;

    data.clear();
    data.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        unsigned int value = 0;

        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> value;

        if (ss.fail())
            return false;

        data.push_back(
            static_cast<unsigned char>(value)
        );
    }

    return true;
}

// ============================================================
// KEY GENERATION
// ============================================================

bool FalconKey::Generate()
{
    publicKey.clear();
    privateKey.clear();

    return Falcon512_KeyGen(
        publicKey,
        privateKey
    );
}

// ============================================================
// DETERMINISTIC KEY GENERATION FROM BERYL SEED
// ============================================================

bool FalconKey::GenerateFromSeed(
    const std::vector<unsigned char>& seed
)
{
    // Falcon-512 deterministic API requires exactly 48 bytes.
    if (seed.size() != 48)
        return false;

    publicKey.clear();
    privateKey.clear();

    return Falcon512_KeyGenFromSeed(
        seed,
        publicKey,
        privateKey
    );
}

// ============================================================
// PUBLIC KEY HEX
// ============================================================

std::string FalconKey::GetPublicKeyHex() const
{
    return ToHex(publicKey);
}

// ============================================================
// PRIVATE KEY HEX
// ============================================================

std::string FalconKey::GetPrivateKeyHex() const
{
    return ToHex(privateKey);
}

// ============================================================
// SET KEYS
// ============================================================

bool FalconKey::SetKeys(
    const std::vector<unsigned char>& pub,
    const std::vector<unsigned char>& priv
)
{
    if (pub.empty())
        return false;

    publicKey = pub;
    privateKey = priv;

    return true;
}

// ============================================================
// SIGN
// ============================================================

std::string FalconKey::Sign(
    const std::string& message
)
{
    if (privateKey.empty())
        return "";

    std::vector<unsigned char> msg(
        message.begin(),
        message.end()
    );

    std::vector<unsigned char> signature;

    if (!Falcon512_Sign(
            privateKey,
            msg,
            signature))
    {
        return "";
    }

    return ToHex(signature);
}

// ============================================================
// VERIFY
// ============================================================

bool FalconKey::Verify(
    const std::string& message,
    const std::string& signature
)
{
    if (publicKey.empty())
        return false;

    if (signature.empty())
        return false;

    std::vector<unsigned char> msg(
        message.begin(),
        message.end()
    );

    std::vector<unsigned char> sig;

    if (!FromHex(
            signature,
            sig))
    {
        return false;
    }

    return Falcon512_Verify(
        publicKey,
        msg,
        sig
    );
}
