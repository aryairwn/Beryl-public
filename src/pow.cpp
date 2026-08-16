// © Arya Irawan — 10 August 2026

#include "pow.h"
#include "yespower/yespower.h"

std::vector<uint8_t> YesPowerHash(
    const std::vector<uint8_t>& input
)
{
    std::vector<uint8_t> output(32);

    yespower_params_t params;

    params.version = YESPOWER_1_0;
    params.N = 2048;
    params.r = 32;
    params.pers = nullptr;
    params.perslen = 0;

    yespower_binary_t hash;

    int ret = yespower_tls(
        input.data(),
        input.size(),
        &params,
        &hash
    );

    if (ret != 0)
    {
        return {};
    }

    for (int i = 0; i < 32; i++)
    {
        output[i] = hash.uc[i];
    }

    return output;
}
