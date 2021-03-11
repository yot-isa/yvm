#include <stdio.h>

#include "cpu.h"

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus)
{
    uint8_t instruction = read(address_bus, cpu->ip);
    printf("inst: %02X\n", instruction);
}
