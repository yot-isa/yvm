#include "rom.h"

uint8_t rom_read(struct Device *device, const uint64_t address)
{
    return device->data.as_rom.data[address];
}

void rom_write(struct Device *device, const uint64_t address, const uint8_t data)
{
}
