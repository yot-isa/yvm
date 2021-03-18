#include <stdio.h>

#include "cpu.h"

#define MASKED(x) ((x) & (uint64_t[]){ 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff }[cpu->type])
#define INCREMENT_IP cpu->ip = MASKED(cpu->ip + 1)

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus)
{
    uint8_t instruction = read(address_bus, cpu->ip);
    uint8_t instruction_type = instruction & 0x7f;
    // size_t instruction_size = (size_t) instruction / 0x10;
    // bool conditional = (bool) (instruction & 0x80) >> 7;
    size_t address_size = (size_t) 1 << cpu->type;
    printf("inst: %02X\n", instruction);

    switch (instruction_type) {
    case 0x00: // Break
        cpu->break_flag = true;
        INCREMENT_IP;
        break;

    case 0x01: { // Increment address stack
        uint8_t carry = 0;
        for (size_t i = 0; i < address_size; ++i) {
            int byte = (int) read(address_bus, MASKED(cpu->asp - i));
            int incremented = byte + 1 + carry;
            carry = ((incremented & 0x100) >> 8);
            uint8_t output_byte = (uint8_t) (incremented & 0xff);
            write(address_bus, MASKED(cpu->asp - i), output_byte);
        }
        INCREMENT_IP;
    } break;

    case 0x02: // Push literal address
        for (size_t i = 0; i < address_size; ++i) {
            INCREMENT_IP;
            uint8_t literal = read(address_bus, cpu->ip);
            write(address_bus, cpu->asp, literal);
            cpu->asp = MASKED(cpu->asp + 1);
        }
        INCREMENT_IP;
        break;

    case 0x10: // No operation
        INCREMENT_IP;
        break;

    case 0x11: { // Add address stack
        uint8_t carry = 0;
        for (size_t i = 0; i < address_size; ++i) {
            cpu->asp = MASKED(cpu->asp - 1);
            int b_byte = (int) read(address_bus, cpu->asp);
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size));
            int sum = a_byte + b_byte + carry;
            carry = ((sum & 0x100) >> 8);
            uint8_t c_byte = (uint8_t) (sum & 0xff);
            write(address_bus, MASKED(cpu->asp - address_size), c_byte);
        }
        INCREMENT_IP;
    } break;
    
    case 0x12: // Drop address stack
        cpu->asp = MASKED(cpu->asp - address_size);
        INCREMENT_IP;
        break;

    case 0x20: { // Jump
        uint64_t new_address = 0;
        for (size_t i = 0; i < address_size; ++i) {
            cpu->asp = MASKED(cpu->asp - 1);
            uint8_t byte = read(address_bus, cpu->asp);
            new_address = (new_address << 8) | byte;
        }
        cpu->ip = MASKED(new_address);
    } break;

    case 0x21: { // Decrement address stack
        uint8_t borrow = 0;
        for (size_t i = 0; i < address_size; ++i) {
            int byte = (int) read(address_bus, MASKED(cpu->asp - i));
            int decremented = byte - 1 - borrow;
            borrow = ((decremented & 0x100) >> 8);
            uint8_t output_byte = (uint8_t) (decremented & 0xff);
            write(address_bus, MASKED(cpu->asp - i), output_byte);
        }
        INCREMENT_IP;
    } break;

    case 0x22: // Duplicate address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t byte = read(address_bus, MASKED(cpu->asp - address_size));
            write(address_bus, cpu->asp, byte);
            cpu->asp = MASKED(cpu->asp + 1);
        }
        INCREMENT_IP;
        break;

    case 0x30: { // Jump to subroutine
        uint64_t new_address = 0;
        uint64_t old_address = cpu->ip + 1;
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t new_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            new_address <<= 8;
            new_address |= new_byte;
            uint8_t old_byte = old_address && 0xff;
            old_address >>= 8;
            write(address_bus, MASKED(cpu->asp - address_size + i), old_byte);
        }
        cpu->ip = MASKED(new_address);
    } break;

    case 0x31: { // Subtract address stack
        uint8_t borrow = 0;
        for (size_t i = 0; i < address_size; ++i) {
            cpu->asp = MASKED(cpu->asp - 1);
            int b_byte = (int) read(address_bus, cpu->asp);
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size));
            int difference = a_byte - b_byte - borrow;
            borrow = ((difference & 0x100) >> 8);
            uint8_t c_byte = (uint8_t) (difference & 0xff);
            write(address_bus, MASKED(cpu->asp - address_size), c_byte);
        }
        INCREMENT_IP;
    } break;

    case 0x32: // Swap address stack
        break;

    case 0x40: // Push next instruction pointer
        break;

    case 0x41: // Address stack equal to zero
        break;

    case 0x42: // Over address stack
        break;

    case 0x50: // Push stack pointer
        break;

    case 0x51: // Address stack not equal to zero
        break;

    case 0x52: // Rotate address stack
        break;

    case 0x60: // Set interrupt disable
        break;

    case 0x61: // Address stack greater than
        break;

    case 0x62: // Counter-rotate address stack
        break;

    case 0x70: // Clear interrupt disable
        break;

    case 0x71: // Address stack less than
        break;

    case 0x72: // Extension
        break;

    default:
        break;
    }
}
