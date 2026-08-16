#include "yespower_wrapper.h"
#include <iostream>


int main()
{
    const char* data="Beryl Genesis";

    uint8_t hash[32];


    if(BerylYesPower(
        (uint8_t*)data,
        strlen(data),
        hash))
    {

        std::cout<<"YESPOWER OK\n";

        for(int i=0;i<32;i++)
            printf("%02x",hash[i]);

        printf("\n");

    }
    else
    {
        std::cout<<"FAILED\n";
    }

}
