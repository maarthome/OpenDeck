#pragma once

#include "application/io/encoders/Encoders.h"

class HWAEncoders : public io::Encoders::HWA
{
    public:
    HWAEncoders() = default;

    MOCK_METHOD3(state, bool(size_t index, uint8_t& numberOfReadings, uint16_t& states));
};