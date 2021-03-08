#include "rom.h"

uint8_t rom_read(struct Device *device, const uint64_t address)
{
    return device->contents.as_rom.data[address];
}

void rom_write(struct Device *device, const uint64_t address, const uint8_t data)
{
}

struct Device create_rom(uint8_t *data)
{
    return struct Device {
        .contents.as_rom = struct Rom_Device {
            .data: data;
        };
        .read = rom_read;
        .write = rom_write;
    };
}
