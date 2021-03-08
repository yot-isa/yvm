#ifndef ROM_H_
#define ROM_H_

#include <stdbool.h>

#include "../address_bus.h"
#include "../device.h"

struct Address_Bus;

struct Device;

struct Rom_Device {
    uint8_t *data;
};

uint8_t rom_read(struct Device *device, const uint64_t address);

void rom_write(struct Device *device, const uint64_t address, const uint8_t data);

bool parse_rom_args(struct Address_Bus *address_bus, int *argc, const char ***argv);

#endif // ROM_H_
