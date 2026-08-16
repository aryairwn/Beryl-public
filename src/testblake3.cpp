#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include "crypto/blake3_wrapper.h"

int main()
{
    std::string msg = "Beryl";

    std::vector<unsigned char> data(
        msg.begin(),
        msg.end()
    );

    std::vector<unsigned char> hash =
        Blake3Hash(data, 20);   // 20 byte = 160 bit

    std::cout << "HASH SIZE : "
              << hash.size()
              << " bytes\n";

    std::cout << "HASH HEX  : ";

    for (unsigned char b : hash)
    {
        std::cout
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int)b;
    }

    std::cout << std::endl;

    return 0;
}
