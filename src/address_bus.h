#ifndef ADDRESS_BUS_H_
#define ADDRESS_BUS_H_

#include <stddef.h>

#include "device.h"

struct Address_Range {
    uint64_t start_address;
    uint64_t end_offset;
};

size_t get_address_range_length(const struct Address_Range *address_range);

struct Device_Address_Range {
    struct Address_Range address_range;
    struct Device *device;
};

#define DEVICES_CAPACITY 64
#define ADDRESS_RANGE_TABLE_CAPACITY DEVICES_CAPACITY

struct Address_Bus {
    struct Device devices[DEVICES_CAPACITY];
    size_t devices_count;
    struct Device_Address_Range address_range_table[ADDRESS_RANGE_TABLE_CAPACITY];
    size_t address_range_table_size;
};

uint8_t read(const struct Address_Bus *address_bus, const uint64_t address);
void write(const struct Address_Bus *address_bus, const uint64_t address, const uint8_t data);
void attach_device(struct Address_Bus *address_bus, struct Device device, struct Address_Range address_range);

#endif // ADDRESS_BUS_H_
