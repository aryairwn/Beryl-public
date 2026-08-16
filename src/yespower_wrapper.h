// © Arya Irawan — 10 August 2026

#ifndef BERYL_YESPOWER_WRAPPER_H
#define BERYL_YESPOWER_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

bool BerylYesPower(
    const uint8_t* input,
    size_t input_len,
    uint8_t output[32]
);

#endif
