#include "address_bus.h"

static bool address_in_range(const uint64_t address, size_t address_range_table_index)
{
    struct Address_Range address_range = address_range_table[address_range_table_index].address_range;
    return address >= address_range.start_address && address <= address_range.start_address + address_range.end_offset;
}

static struct Device_Address_Range *device_address_range_search(const uint64_t address)
{
    // TODO: Implement binary search for device address range searching
    for (size_t i = 0; i < address_range_table_size; ++i) {
        if (address_in_range(address, i)) {
            return &address_range_table[i];
        }
    }

    return NULL;
}

uint8_t read(const uint64_t address)
{
    struct Device_Address_Range *device_address_range = device_address_range_search(address);
    if (device_address_range) {
        uint64_t offsetted_address = address - device_address_range->address_range.start_address;
        struct Device *device = device_address_range->device;
        return device->read(device, offsetted_address);
    }

    return 0;
}

void write(const uint64_t address, const uint8_t data)
{
    struct Device_Address_Range *device_address_range = device_address_range_search(address);
    if (device_address_range) {
        uint64_t offsetted_address = address - device_address_range->address_range.start_address;
        struct Device *device = device_address_range->device;
        device->write(device, offsetted_address, data);
    }
}
