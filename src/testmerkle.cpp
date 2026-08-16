#include "merkle.h"
#include "transaction.h"

#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::cout << "=== BERYL MERKLE TEST ===\n";

    // --------------------------------------------------------
    // TX 1
    // --------------------------------------------------------

    BerylTransaction tx1;

    TxOutput out1;
    out1.address = "berA";
    out1.amount = 100;
    out1.spent = false;

    tx1.vout.push_back(out1);
    tx1.txid = CalculateTxID(tx1);

    // --------------------------------------------------------
    // TX 2
    // --------------------------------------------------------

    BerylTransaction tx2;

    TxOutput out2;
    out2.address = "berB";
    out2.amount = 200;
    out2.spent = false;

    tx2.vout.push_back(out2);
    tx2.txid = CalculateTxID(tx2);

    // --------------------------------------------------------
    // TX 3
    // --------------------------------------------------------

    BerylTransaction tx3;

    TxOutput out3;
    out3.address = "berC";
    out3.amount = 300;
    out3.spent = false;

    tx3.vout.push_back(out3);
    tx3.txid = CalculateTxID(tx3);

    // --------------------------------------------------------
    // TEST 1: satu transaksi
    // --------------------------------------------------------

    std::vector<BerylTransaction> one = {
        tx1
    };

    std::string root1 =
        CalculateMerkleRoot(one);

    if (root1 != tx1.txid)
    {
        std::cout
            << "SINGLE TX ROOT FAIL\n";
        return 1;
    }

    std::cout
        << "SINGLE TX ROOT OK\n";

    // --------------------------------------------------------
    // TEST 2: dua transaksi
    // --------------------------------------------------------

    std::vector<BerylTransaction> two = {
        tx1,
        tx2
    };

    std::string root2 =
        CalculateMerkleRoot(two);

    if (root2.empty())
    {
        std::cout
            << "TWO TX ROOT FAIL\n";
        return 1;
    }

    if (root2 == tx1.txid ||
        root2 == tx2.txid)
    {
        std::cout
            << "TWO TX ROOT COLLISION FAIL\n";
        return 1;
    }

    if (root2.size() != 64)
    {
        std::cout
            << "TWO TX ROOT SIZE FAIL\n";
        return 1;
    }

    std::cout
        << "TWO TX ROOT OK\n";

    // --------------------------------------------------------
    // TEST 3: tiga transaksi
    // --------------------------------------------------------

    std::vector<BerylTransaction> three = {
        tx1,
        tx2,
        tx3
    };

    std::string root3 =
        CalculateMerkleRoot(three);

    if (root3.empty())
    {
        std::cout
            << "THREE TX ROOT FAIL\n";
        return 1;
    }

    if (root3.size() != 64)
    {
        std::cout
            << "THREE TX ROOT SIZE FAIL\n";
        return 1;
    }

    std::cout
        << "THREE TX ROOT OK\n";

    // --------------------------------------------------------
    // TEST 4: urutan transaksi harus memengaruhi root
    // --------------------------------------------------------

    std::vector<BerylTransaction> reversed = {
        tx2,
        tx1
    };

    std::string reversedRoot =
        CalculateMerkleRoot(reversed);

    if (reversedRoot == root2)
    {
        std::cout
            << "ORDER SENSITIVITY FAIL\n";
        return 1;
    }

    std::cout
        << "ORDER SENSITIVITY OK\n";

    // --------------------------------------------------------
    // TEST 5: perubahan transaksi harus mengubah root
    // --------------------------------------------------------

    BerylTransaction modifiedTx2 = tx2;

    modifiedTx2.vout[0].amount = 201;
    modifiedTx2.txid =
        CalculateTxID(modifiedTx2);

    std::vector<BerylTransaction> modified = {
        tx1,
        modifiedTx2
    };

    std::string modifiedRoot =
        CalculateMerkleRoot(modified);

    if (modifiedRoot == root2)
    {
        std::cout
            << "DATA SENSITIVITY FAIL\n";
        return 1;
    }

    std::cout
        << "DATA SENSITIVITY OK\n";

    // --------------------------------------------------------
    // TEST 6: empty block transaction list
    // --------------------------------------------------------

    std::vector<BerylTransaction> empty;

    if (!CalculateMerkleRoot(empty).empty())
    {
        std::cout
            << "EMPTY ROOT FAIL\n";
        return 1;
    }

    std::cout
        << "EMPTY ROOT OK\n";

    std::cout
        << "\nALL BERYL MERKLE TESTS PASSED\n";

    return 0;
}
