#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../error.h"
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

static uint8_t *slurp_file(const char *file_path, size_t *size)
{
    FILE *f = NULL;
    uint8_t *buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) goto end;
    if (fseek(f, 0, SEEK_END) < 0) goto end;

    long file_size = ftell(f);
    if (file_size < 0) goto end;

    buffer = malloc((size_t) file_size);
    if (buffer == NULL) goto end;

    if (fseek(f, 0, SEEK_SET) < 0) goto end;

    fread(buffer, 1, (size_t) file_size, f);
    if (ferror(f) < 0) goto end;

    *size = (size_t) file_size;

end:
    if (f) fclose(f);
    return buffer;
}

void rom_parse_args(struct Address_Bus *address_bus, int *argc, const char ***argv)
{
    if (*argc == 0) {
        exit_with_error("Address range expected.");
    }

    const char *address_range_string = shift(argc, argv);
    struct Address_Range address_range = parse_address_range(address_range_string);
    size_t address_range_length = get_address_range_length(&address_range);

    if (*argc == 0) {
        exit_with_error("ROM file path expected.");
    }
    
    const char *file_path = shift(argc, argv);
    size_t file_size;
    struct Rom_Device *rom = malloc(sizeof(struct Rom_Device));
    rom->data = slurp_file(file_path, &file_size);

    if (rom->data == NULL) {
        exit_with_error("Could not read file `%s`: %s.", file_path, strerror(errno));
    }

    if (file_size != address_range_length) {
        exit_with_error(
            "The file size (%zu bytes) does not match the specified address range length (%zu bytes).",
            file_size,
            address_range_length
        );
    }

    struct Device device = {
        .number = address_bus->devices_count,
        .type = DEVICE_ROM,
        .contents.as_rom = rom,
        .read = rom_read,
        .write = rom_write
    };

    attach_device(address_bus, device, address_range);
}
