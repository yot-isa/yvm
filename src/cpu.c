#include <stdio.h>

#include "cpu.h"

void cpu_initialize(struct Cpu *cpu, struct Address_Bus *address_bus, enum Yot_Type yot_type)
{
    size_t address_size = (size_t) 1 << yot_type;

    uint64_t sp = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, i);
        sp = (sp << 8) | byte;
    }
    uint64_t ip = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, address_size * 2 + i);
        ip = (ip << 8) | byte;
    }

    *cpu = (struct Cpu) {
        .type = yot_type,
        .ip = ip,
        .sp = sp,
        .irp = 0,
        .break_flag = false
    };
}

#define MASKED(x) ((x) & (uint64_t[]){ 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff }[cpu->type])
#define INCREMENT_IP cpu->ip = MASKED(cpu->ip + 1)
#define SET_BREAK_FLAG cpu->break_flag = true
#define GET_ADDRESS uint64_t address = 0;\
    for (size_t i = 0; i < address_size; ++i)\
        address = (address << 8) | read(address_bus, MASKED(cpu->sp - address_size + i))
#define DROP_ADDRESS cpu->sp = MASKED(cpu->sp - address_size)

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus)
{
    uint8_t instruction = read(address_bus, cpu->ip);
    size_t address_size = (size_t) 1 << cpu->type;
    printf("inst: %2X\n", instruction);

    switch (instruction) {
    case 0x00: // Break
        SET_BREAK_FLAG;
        INCREMENT_IP;
        return;

    case 0x10: // No operation
        INCREMENT_IP;
        return;
        
    case 0x20: // Push literal
        write(address_bus, MASKED(cpu->sp), read(address_bus, MASKED(cpu->ip + 1)));
        cpu->sp = MASKED(cpu->sp + 1);
        cpu->ip = MASKED(cpu->ip + 2);
        return;
    
    case 0x30: // Drop
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
        return;

    case 0x40: { // Switch
        size_t offset = (size_t) read(address_bus, MASKED(cpu->sp - 1));
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 2));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2 - offset));
        write(address_bus, MASKED(cpu->sp - 2), b_byte);
        write(address_bus, MASKED(cpu->sp - 2 - offset), a_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0x41: { // Pick
        size_t offset = (size_t) read(address_bus, MASKED(cpu->sp - 1));
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 2 - offset));
        write(address_bus, MASKED(cpu->sp - 1), byte);
        INCREMENT_IP;
    } return;

    case 0x42: { // Paste
        size_t offset = (size_t) read(address_bus, MASKED(cpu->sp - 1));
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 3 - offset), byte);
        cpu->sp = MASKED(cpu->sp - 2);
        INCREMENT_IP;
    } return;

    case 0x50: { // Roll
        size_t offset = (size_t) read(address_bus, MASKED(cpu->sp - 1));
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 2 - offset));
        for (size_t i = 0; i < offset + 1; ++i) {
            uint8_t temp = read(address_bus, MASKED(cpu->sp - 2 - i));
            write(address_bus, MASKED(cpu->sp - 2 - i), byte);
            byte = temp;
        }
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;
    
    case 0x51: { // Insert
        size_t offset = (size_t) read(address_bus, MASKED(cpu->sp - 1));
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 2));
        for (size_t i = 0; i < offset + 2; ++i) {
            uint8_t temp = read(address_bus, MASKED(cpu->sp - 3 - offset + i));
            write(address_bus, MASKED(cpu->sp - 3 - offset + i), byte);
            byte = temp;
        }
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;
    
    case 0x60: { // Fetch memory
        GET_ADDRESS;
        DROP_ADDRESS;
        write(address_bus, cpu->sp, read(address_bus, address));
        cpu->sp = MASKED(cpu->sp + 1);
        INCREMENT_IP;
    } return;

    case 0x61: { // Store memory
        GET_ADDRESS;
        DROP_ADDRESS;
        write(address_bus, address, read(address_bus, MASKED(cpu->sp - 1)));
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0x70: { // Jump
        GET_ADDRESS;
        DROP_ADDRESS;
        cpu->ip = address;
    } return;

    case 0x71: { // Branch
        GET_ADDRESS;
        DROP_ADDRESS;
        uint8_t pred = read(address_bus, MASKED(cpu->sp - 1));
        if (pred)
            cpu->ip = address;
        else
            INCREMENT_IP;
    } return;

    case 0x72: { // Jump to subroutine
        uint64_t dropped_address = 0;
        uint64_t pushed_address = MASKED(cpu->ip + 1);
        for (size_t i = 0; i < address_size; ++i) {
            dropped_address = (dropped_address << 8) | read(address_bus, MASKED(cpu->sp - address_size + i));
            write(address_bus, MASKED(cpu->sp - 1 - i), pushed_address & 0xff);
            pushed_address >>= 8;
        }
        cpu->ip = dropped_address;
    } return;

    case 0x80: // Set interrupt flag
        cpu->interrupt_disable_flag = (bool) read(address_bus, MASKED(cpu->sp - 1));
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
        return;

    case 0x81: { // Set interrupt address
        GET_ADDRESS;
        DROP_ADDRESS;
        cpu->irp = address;
        INCREMENT_IP;
    } return;
    
    case 0x90: { // Add
        int b_byte = (int) read(address_bus, MASKED(cpu->sp - 1));
        int a_byte = (int) read(address_bus, MASKED(cpu->sp - 2));
        int sum = a_byte + b_byte;
        write(address_bus, MASKED(cpu->sp - 2), (uint8_t) sum);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;
    
    case 0x91: { // Subtract
        int b_byte = (int) read(address_bus, MASKED(cpu->sp - 1));
        int a_byte = (int) read(address_bus, MASKED(cpu->sp - 2));
        int difference = a_byte - b_byte;
        write(address_bus, MASKED(cpu->sp - 2), (uint8_t) difference);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xa0: { // Add with carry
        uint8_t carry = read(address_bus, MASKED(cpu->sp - 1)) ? 0x01 : 0x00;
        int b_byte = (int) read(address_bus, MASKED(cpu->sp - 2));
        int a_byte = (int) read(address_bus, MASKED(cpu->sp - 3));
        int sum = a_byte + b_byte + carry;
        uint8_t overflow = ((sum & 0x100) >> 8);
        write(address_bus, MASKED(cpu->sp - 3), (uint8_t) (sum & 0xff));
        write(address_bus, MASKED(cpu->sp - 2), overflow);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xa1: { // Subtract with borrow
        uint8_t borrow = read(address_bus, MASKED(cpu->sp - 1)) ? 0x01 : 0x00;
        int b_byte = (int) read(address_bus, MASKED(cpu->sp - 2));
        int a_byte = (int) read(address_bus, MASKED(cpu->sp - 3));
        int sum = a_byte - b_byte - borrow;
        uint8_t overflow = ((sum & 0x100) >> 8);
        write(address_bus, MASKED(cpu->sp - 3), (uint8_t) (sum & 0xff));
        write(address_bus, MASKED(cpu->sp - 2), overflow);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xb0: { // And
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 1));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 2), a_byte & b_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xb1: { // Inclusive or
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 1));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 2), a_byte | b_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xb2: { // Exclusive or
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 1));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 2), a_byte ^ b_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xc0: { // Shift left
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 1));
        write(address_bus, MASKED(cpu->sp - 1), (uint8_t) (byte << 1));
        INCREMENT_IP;
    } return;

    case 0xc1: { // Shift right
        uint8_t byte = read(address_bus, MASKED(cpu->sp - 1));
        write(address_bus, MASKED(cpu->sp - 1), byte >> 1);
        INCREMENT_IP;
    } return;

    case 0xd0: { // Equal
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 1));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 2), (uint8_t) a_byte == b_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    case 0xd1: { // Not equal
        uint8_t a_byte = read(address_bus, MASKED(cpu->sp - 1));
        uint8_t b_byte = read(address_bus, MASKED(cpu->sp - 2));
        write(address_bus, MASKED(cpu->sp - 2), (uint8_t) a_byte != b_byte);
        cpu->sp = MASKED(cpu->sp - 1);
        INCREMENT_IP;
    } return;

    default:
        INCREMENT_IP;
        return;
    }
}
