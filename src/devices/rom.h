#include "../device.h"

struct Device;

struct Rom_Device {
    uint8_t *data;
};

uint8_t rom_read(struct Device *device, const uint64_t address);

void rom_write(struct Device *device, const uint64_t address, const uint8_t data);
