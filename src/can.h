#pragma once
#include <cstdint>

struct can_msg {
    uint32_t id;
    uint32_t dlc;
    union {
        uint8_t data[8];
        uint32_t data32[2];
    };
};

void can_task(void* params);