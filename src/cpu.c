#include <stdio.h>

#include "cpu.h"

#define MASKED(x) ((x) & (uint64_t[]){ 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff }[cpu->type])
#define INCREMENT_IP cpu->ip = MASKED(cpu->ip + 1)

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus)
{
    uint8_t instruction = read(address_bus, cpu->ip);
    uint8_t instruction_type = instruction & 0x1f;
    size_t instruction_size = (size_t) 1 << ((instruction & 0x60) >> 5);
    printf("inst: %02X\n", instruction);

    switch (instruction_type) {
    case 0x00: // Halt
        cpu->halt = true;
        INCREMENT_IP;
        break;

    case 0x01: // Push
        for (size_t i = 0; i < instruction_size; ++i) {
            INCREMENT_IP;
            uint8_t literal = read(address_bus, cpu->ip);
            write(address_bus, cpu->sp, literal);
            cpu->sp = MASKED(cpu->sp + 1);
        }
        INCREMENT_IP;
        break;

    case 0x02: // Pop
        cpu->sp = MASKED(cpu->sp - instruction_size);
        INCREMENT_IP;
        break;

    case 0x03: { // Jump
        uint64_t new_address = 0;
        for (size_t i = 0; i < instruction_size; ++i) {
            cpu->sp = MASKED(cpu->sp - 1);
            uint8_t byte = read(address_bus, cpu->sp);
            new_address = (new_address << 8) | byte;
        }
        cpu->ip = MASKED(new_address);
    } break;
    
    case 0x04: { // Jump to subroutine
        uint64_t new_address = 0;
        uint64_t old_address = cpu->ip + 1;
        for (size_t i = 0; i < instruction_size; ++i) {
            uint8_t new_byte = read(address_bus, MASKED(cpu->sp - 8 + i));
            new_address <<= 8;
            new_address |= new_byte;
            uint8_t old_byte = old_address && 0xff;
            old_address >>= 8;
            write(address_bus, MASKED(cpu->sp - 8 + i), old_byte);
        }
        cpu->ip = MASKED(new_address);
    } break;

    case 0x05: { // Add
        uint8_t carry = 0;
        for (size_t i = 0; i < instruction_size; ++i) {
            cpu->sp = MASKED(cpu->sp - 1);
            int b_byte = (int) read(address_bus, cpu->sp);
            int a_byte = (int) read(address_bus, MASKED(cpu->sp - 4));
            int sum = a_byte + b_byte + carry;
            carry = ((sum & 0x100) >> 8);
            uint8_t c_byte = (uint8_t) (sum & 0xff);
            write(address_bus, MASKED(cpu->sp - 4), c_byte);
        }
        cpu->carry = (bool) carry;
        INCREMENT_IP;
    } break;
    }
}
