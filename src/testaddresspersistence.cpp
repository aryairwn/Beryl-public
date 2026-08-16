#include "wallet.h"

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>

int main()
{
    std::cout << "=== BERYL ADDRESS PERSISTENCE TEST ===\n";

    const std::string sourceWallet =
        "../data/wallet.dat";

    const std::string testWallet =
        "../data/test_wallet_persistence.dat";

    // ============================================================
    // 1. LOAD WALLET UTAMA
    // ============================================================

    BerylWallet wallet;

    if(!wallet.Load(sourceWallet))
    {
        std::cerr << "FAIL: MAIN WALLET LOAD\n";
        return 1;
    }

    std::string mainAddress =
        wallet.GetAddress();

    std::cout
        << "MAIN ADDRESS: "
        << mainAddress
        << "\n";

    // ============================================================
    // 2. GENERATE ADDRESS BARU
    // ============================================================

    std::string newAddress;

    if(!wallet.GenerateNewAddress(newAddress))
    {
        std::cerr
            << "FAIL: GENERATE NEW ADDRESS\n";
        return 1;
    }

    std::cout
        << "NEW ADDRESS: "
        << newAddress
        << "\n";

    // ============================================================
    // 3. CEK ADDRESS ADA DI MEMORY
    // ============================================================

    auto addressesBefore =
        wallet.GetAddresses();

    bool foundBefore = false;

    for(const auto& addr : addressesBefore)
    {
        if(addr == newAddress)
        {
            foundBefore = true;
            break;
        }
    }

    if(!foundBefore)
    {
        std::cerr
            << "FAIL: NEW ADDRESS NOT FOUND BEFORE SAVE\n";
        return 1;
    }

    std::cout
        << "NEW ADDRESS FOUND BEFORE SAVE\n";

    // ============================================================
    // 4. SAVE KE FILE TEST
    // ============================================================

    if(!wallet.Save(testWallet))
    {
        std::cerr
            << "FAIL: SAVE TEST WALLET\n";
        return 1;
    }

    std::cout
        << "SAVE OK\n";

    // ============================================================
    // 5. LOAD DENGAN OBJECT WALLET BARU
    // ============================================================

    BerylWallet restored;

    if(!restored.Load(testWallet))
    {
        std::cerr
            << "FAIL: RESTORE TEST WALLET\n";
        std::remove(testWallet.c_str());
        return 1;
    }

    std::cout
        << "RESTORE OK\n";

    // ============================================================
    // 6. MAIN ADDRESS HARUS SAMA
    // ============================================================

    if(restored.GetAddress() != mainAddress)
    {
        std::cerr
            << "FAIL: MAIN ADDRESS CHANGED\n";
        std::remove(testWallet.c_str());
        return 1;
    }

    std::cout
        << "MAIN ADDRESS RESTORED OK\n";

    // ============================================================
    // 7. CEK ADDRESS BARU SETELAH RESTORE
    // ============================================================

    auto addressesAfter =
        restored.GetAddresses();

    bool foundAfter = false;

    for(const auto& addr : addressesAfter)
    {
        std::cout
            << "RESTORED ADDRESS: "
            << addr
            << "\n";

        if(addr == newAddress)
        {
            foundAfter = true;
        }
    }

    if(!foundAfter)
    {
        std::cerr
            << "FAIL: NEW ADDRESS LOST AFTER RESTORE\n";
        std::remove(testWallet.c_str());
        return 1;
    }

    std::cout
        << "NEW ADDRESS RESTORED OK\n";

    // ============================================================
    // 8. CLEANUP
    // ============================================================

    std::remove(testWallet.c_str());

    std::cout
        << "TEST FILE REMOVED OK\n";

    std::cout
        << "ALL ADDRESS PERSISTENCE TESTS PASSED\n";

    return 0;
}
