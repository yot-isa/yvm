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

static uint8_t *slurp_file(const char *file_path)
{
    FILE *f = NULL;
    uint8_t *buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) goto end;
    if (fseek(f, 0, SEEK_END) < 0) goto end;

    long size = ftell(f);
    if (size < 0) goto end;

    buffer = malloc((size_t) size + 1);
    if (buffer == NULL) goto end;

    if (fseek(f, 0, SEEK_SET) < 0) goto end;

    fread(buffer, 1, (size_t) size, f);
    if (ferror(f) < 0) goto end;

    buffer[size] = '\0';

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
    struct Address_Range address_range = parse_address_range(address_bus, address_range_string);

    if (*argc == 0) {
        exit_with_error("ROM file path expected.");
    }
    
    const char *file_path = shift(argc, argv);
    struct Rom_Device *rom = malloc(sizeof(struct Rom_Device));
    rom->data = slurp_file(file_path);

    if (rom->data == NULL) {
        exit_with_error("Could not read file `%s`: %s.", file_path, strerror(errno));
    }

    struct Device device = {
        .contents.as_rom = rom,
        .read = rom_read,
        .write = rom_write
    };

    attach_device(address_bus, device, address_range);
}
