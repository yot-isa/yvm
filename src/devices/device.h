#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h>

#include "rom.h"
#include "ram.h"

struct Device {
    size_t number;
    enum {
        DEVICE_ROM,
        DEVICE_RAM
    } type;
    union {
        struct Rom_Device *as_rom;
        struct Ram_Device *as_ram;
    } contents;
    uint8_t (*read)(struct Device *device, const uint64_t address);
    void (*write)(struct Device *device, const uint64_t address, const uint8_t data);
};

#endif // DEVICE_H_
