// © Arya Irawan — 10 August 2026

#include "yespower_wrapper.h"
#include "yespower/yespower.h"
#include <string.h>

bool BerylYesPower(
    const uint8_t* input,
    size_t input_len,
    uint8_t output[32]
)
{
    yespower_params_t params;

    params.version = YESPOWER_1_0;
    params.N = 2048;
    params.r = 32;
    params.pers = NULL;
    params.perslen = 0;

    yespower_binary_t result;


    if (yespower_tls(
        input,
        input_len,
        &params,
        &result
    ) != 0)
    {
        return false;
    }


    memcpy(
        output,
        result.uc,
        32
    );

    return true;
}
