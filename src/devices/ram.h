#ifndef RAM_H_
#define RAM_H_

#include <stdbool.h>

#include "../address_bus.h"
#include "device.h"

struct Address_Bus;
struct Device;

struct Ram_Device {
    uint8_t *data;
};

uint8_t ram_read(struct Device *device, const uint64_t address);

void ram_write(struct Device *device, const uint64_t address, const uint8_t data);

void ram_parse_args(struct Address_Bus *address_bus, int *argc, const char ***argv);

#endif // RAM_H_
