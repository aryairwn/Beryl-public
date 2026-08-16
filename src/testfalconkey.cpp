#include "crypto/falcon/falcon.h"
#include <iostream>
#include <vector>

int main()
{
    FalconKey original;

    // Generate key
    if (!original.Generate())
    {
        std::cout << "KEYGEN FAILED\n";
        return 1;
    }

    std::cout << "KEYGEN OK\n";

    // Message
    std::string message =
        "Beryl transaction test";

    // Sign
    std::string signature =
        original.Sign(message);

    if (signature.empty())
    {
        std::cout << "SIGN FAILED\n";
        return 1;
    }

    std::cout << "SIGN OK\n";
    std::cout << "SIGNATURE HEX SIZE: "
              << signature.size()
              << "\n";

    // Ambil raw keys
    std::vector<unsigned char> pub =
        original.GetPublicKeyRaw();

    std::vector<unsigned char> priv =
        original.GetPrivateKeyRaw();

    std::cout << "PUBLIC KEY SIZE: "
              << pub.size()
              << "\n";

    std::cout << "PRIVATE KEY SIZE: "
              << priv.size()
              << "\n";

    // Buat object baru
    FalconKey verifier;

    if (!verifier.SetKeys(pub, priv))
    {
        std::cout << "SETKEYS FAILED\n";
        return 1;
    }

    std::cout << "SETKEYS OK\n";

    // Verify
    if (!verifier.Verify(
            message,
            signature))
    {
        std::cout << "VERIFY FAILED\n";
        return 1;
    }

    std::cout << "VERIFY OK\n";

    // Test tampering
    if (verifier.Verify(
            "Beryl transaction tampered",
            signature))
    {
        std::cout << "TAMPER TEST FAILED\n";
        return 1;
    }

    std::cout << "TAMPER TEST OK\n";

    return 0;
}
