#ifndef BERYL_BLAKE3_WRAPPER_H
#define BERYL_BLAKE3_WRAPPER_H

#include <vector>
#include <cstddef>

std::vector<unsigned char> Blake3Hash(
    const std::vector<unsigned char>& data,
    size_t outlen
);

#endif
