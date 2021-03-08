#include "device.h"

#define DEVICES_CAPACITY 64
#define ADDRESS_RANGE_TABLE_CAPACITY DEVICES_CAPACITY

struct Address_Range {
    uint64_t start_address;
    uint64_t end_offset;
};

struct Device_Address_Range {
    struct Address_Range address_range;
    struct Device *device;
};

struct Device devices[DEVICES_CAPACITY];
struct Device_Address_Range address_range_table[ADDRESS_RANGE_TABLE_CAPACITY];
size_t address_range_table_size = 0;

uint8_t read(const uint64_t address);

void write(const uint64_t address, const uint8_t data);
