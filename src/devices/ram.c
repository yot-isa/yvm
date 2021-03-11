#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../error.h"
#include "ram.h"
#include "../parse_utils.h"
#include "../address_bus.h"

uint8_t ram_read(struct Device *device, const uint64_t address)
{
    return device->contents.as_ram->data[address];
}

void ram_write(struct Device *device, const uint64_t address, const uint8_t data)
{
    device->contents.as_ram->data[address] = data;
}

void ram_parse_args(struct Address_Bus *address_bus, int *argc, const char ***argv)
{
    if (*argc == 0) {
        exit_with_error("Address range expected.");
    }

    const char *address_range_string = shift(argc, argv);
    struct Address_Range address_range = parse_address_range(address_range_string);
    size_t address_range_length = get_address_range_length(&address_range);

    struct Ram_Device *ram = malloc(sizeof(struct Ram_Device));
    ram->data = malloc(address_range_length * sizeof(uint8_t));
    memset(ram->data, 0, address_range_length * sizeof(uint8_t));

    struct Device device = {
        .number = address_bus->devices_count,
        .type = DEVICE_RAM,
        .contents.as_ram = ram,
        .read = ram_read,
        .write = ram_write
    };

    attach_device(address_bus, device, address_range);
}
