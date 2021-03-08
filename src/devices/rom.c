#include <stdio.h>
#include <stdlib.h>

#include "rom.h"
#include "../parse_utils.h"
#include "../address_bus.h"

uint8_t rom_read(struct Device *device, const uint64_t address)
{
    return device->contents.as_rom->data[address];
}

void rom_write(struct Device *device, const uint64_t address, const uint8_t data)
{
    if (device || address || data) {};
}

bool parse_rom_args(struct Address_Bus *address_bus, int *argc, const char ***argv)
{
    if (*argc == 0) {
        return true;
    }

    const char *address_range_string = shift(argc, argv);
    struct Address_Range address_range;
    struct Address_Range_Parse_Error address_range_parse_error;
    bool has_errored = parse_address_range(address_range_string, &address_range, &address_range_parse_error);
    if (has_errored) {
        return true; 
    }

    if (*argc == 0) {
        return true;
    }

    const char *file_path = shift(argc, argv);

    FILE *file_pointer = fopen(file_path, "rb");
    
    if (!file_pointer) {
        // file does not exist
        return true;
    }
    
    fseek(file_pointer, 0, SEEK_END);
    size_t file_size = (size_t) ftell(file_pointer);
    fseek(file_pointer, 0, SEEK_SET);
    uint8_t *buffer = malloc(file_size);

    if (!buffer) {
        return true;
    }

    if (fread(&buffer, sizeof(*buffer), file_size, file_pointer) != file_size) {
        return true;
    }

    fclose(file_pointer);

    struct Rom_Device *rom = malloc(sizeof(struct Rom_Device));
    if (!rom) {
        return true;
    }
    rom->data = buffer;
    
    struct Device device = {
        .contents.as_rom = rom,
        .read = rom_read,
        .write = rom_write
    };

    attach_device(address_bus, device, address_range);

    return false;
}
