#include "coinbase.h"
#include "merkle.h"
#include "transaction.h"

#include <iostream>
#include <string>

int main()
{
    std::cout << "=== BERYL COINBASE UNIQUENESS TEST ===\n";

    BerylTransaction cb2 =
        CreateCoinbaseTransaction(
            "berTESTMINER",
            40ULL * 100000000ULL,
            "BERYL-COINBASE-HEIGHT-2"
        );

    BerylTransaction cb3 =
        CreateCoinbaseTransaction(
            "berTESTMINER",
            40ULL * 100000000ULL,
            "BERYL-COINBASE-HEIGHT-3"
        );

    std::cout << "HEIGHT 2 DATA: "
              << cb2.coinbaseData << "\n";

    std::cout << "HEIGHT 2 TXID: "
              << cb2.txid << "\n";

    std::cout << "HEIGHT 3 DATA: "
              << cb3.coinbaseData << "\n";

    std::cout << "HEIGHT 3 TXID: "
              << cb3.txid << "\n";

    if (cb2.txid == cb3.txid)
    {
        std::cout << "TXID UNIQUENESS FAIL\n";
        return 1;
    }

    std::cout << "TXID UNIQUE OK\n";

    std::vector<BerylTransaction> txs2;
    txs2.push_back(cb2);

    std::vector<BerylTransaction> txs3;
    txs3.push_back(cb3);

    const std::string merkle2 =
        CalculateMerkleRoot(txs2);

    const std::string merkle3 =
        CalculateMerkleRoot(txs3);

    std::cout << "HEIGHT 2 MERKLE: "
              << merkle2 << "\n";

    std::cout << "HEIGHT 3 MERKLE: "
              << merkle3 << "\n";

    if (merkle2 == merkle3)
    {
        std::cout << "MERKLE UNIQUENESS FAIL\n";
        return 1;
    }

    std::cout << "MERKLE UNIQUE OK\n";
    std::cout << "\nALL BERYL COINBASE UNIQUENESS TESTS PASSED\n";

    return 0;
}
