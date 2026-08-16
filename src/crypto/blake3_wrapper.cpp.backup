#include "blake3_wrapper.h"

extern "C" {
#include "BLAKE3/c/blake3.h"
}

std::vector<unsigned char> Blake3Hash(
    const std::vector<unsigned char>& data,
    size_t outlen
)
{
    blake3_hasher hasher;

    blake3_hasher_init(&hasher);

    if (!data.empty())
    {
        blake3_hasher_update(
            &hasher,
            data.data(),
            data.size()
        );
    }

    std::vector<unsigned char> hash(outlen);

    blake3_hasher_finalize(
        &hasher,
        hash.data(),
        outlen
    );

    return hash;
}
