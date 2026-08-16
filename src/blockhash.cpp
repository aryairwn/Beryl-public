// © Arya Irawan — 10 August 2026

#include "blockhash.h"
#include "yespower_wrapper.h"

#include <sstream>
#include <iomanip>


std::string SerializeHeader(
    const BerylHeader& header
)
{

    std::stringstream ss;


    ss
    << header.version
    << header.previousHash
    << header.merkleRoot
    << header.utxoRoot
    << header.contractRoot
    << header.timestamp
    << header.nonce
    << header.difficulty
    << header.height;


    return ss.str();

}



std::string GetBlockHash(
    const BerylHeader& header
)
{

    std::string data =
    SerializeHeader(header);


    uint8_t hash[32];


    BerylYesPower(
        (uint8_t*)data.data(),
        data.size(),
        hash
    );


    std::stringstream ss;


    for(int i=0;i<32;i++)
    {
        ss
        << std::hex
        << std::setw(2)
        << std::setfill('0')
        << (int)hash[i];
    }


    return ss.str();

}
