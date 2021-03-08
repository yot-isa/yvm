#pragma once

#include <stdint.h>

#include "devices/rom.h"

struct Device {
    union {
        struct Rom_Device as_rom;
    } contents;
    uint8_t (*read)(struct Device *device, const uint64_t address);
    void (*write)(struct Device *device, const uint64_t address, const uint8_t data);
};
