#include "address_bus.h"
#include "parse_utils.h"
#include "error.h"

size_t get_address_range_length(const struct Address_Range *address_range)
{
    return address_range->end_offset + 1;
}

static bool address_in_range(const struct Address_Bus *address_bus, const uint64_t address, size_t address_range_table_index)
{
    struct Address_Range address_range = address_bus->address_range_table[address_range_table_index].address_range;
    return address >= address_range.start_address && address <= address_range.start_address + address_range.end_offset;
}

static const struct Device_Address_Range *device_address_range_search(const struct Address_Bus *address_bus, const uint64_t address)
{
    // TODO(#1): Implement binary search for device address range searching
    for (size_t i = 0; i < address_bus->address_range_table_size; ++i) {
        if (address_in_range(address_bus, address, i)) {
            return &(address_bus->address_range_table[i]);
        }
    }

    return NULL;
}

uint8_t read(const struct Address_Bus *address_bus, const uint64_t address)
{
    const struct Device_Address_Range *device_address_range = device_address_range_search(address_bus, address);
    if (device_address_range) {
        uint64_t offsetted_address = address - device_address_range->address_range.start_address;
        struct Device *device = device_address_range->device;
        return device->read(device, offsetted_address);
    }

    return 0;
}

void write(const struct Address_Bus *address_bus, const uint64_t address, const uint8_t data)
{
    const struct Device_Address_Range *device_address_range = device_address_range_search(address_bus, address);
    if (device_address_range) {
        uint64_t offsetted_address = address - device_address_range->address_range.start_address;
        struct Device *device = device_address_range->device;
        device->write(device, offsetted_address, data);
    }
}

static bool address_ranges_overlap(struct Address_Range *a, struct Address_Range *b)
{
    return a->start_address <= b->start_address + b->end_offset && b->start_address <= a->start_address + a->end_offset;
}

void attach_device(struct Address_Bus *address_bus, struct Device device, struct Address_Range address_range)
{
    for (size_t i = 0; i < address_bus->address_range_table_size; ++i) {
        if (address_ranges_overlap(&address_range, &address_bus->address_range_table[i].address_range)) {
            exit_with_error(
                "Address range overlaps an already occupied address range of device #%zu.",
                address_bus->address_range_table[i].device->number
            );
        }
    }

    address_bus->devices[address_bus->devices_count] = device;
    struct Device *device_pointer = &(address_bus->devices[address_bus->devices_count]);
    ++address_bus->devices_count;

    // TODO: Sort address ranges in the address range table
    address_bus->address_range_table[address_bus->address_range_table_size++] = (struct Device_Address_Range) {
        .device = device_pointer,
        .address_range = address_range
    };
}
