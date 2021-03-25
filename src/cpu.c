#include <stdio.h>

#include "cpu.h"

#define MASKED(x) ((x) & (uint64_t[]){ 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff }[cpu->type])
#define INCREMENT_IP cpu->ip = MASKED(cpu->ip + 1)

void cpu_initialize(struct Cpu *cpu, struct Address_Bus *address_bus, enum Yot_Type yot_type)
{
    size_t address_size = (size_t) 1 << cpu->type;
    
    uint64_t dsp = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, i);
        dsp = (dsp << 8) | byte;
    }
    uint64_t asp = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, address_size - 1 + i);
        asp = (asp << 8) | byte;
    }
    uint64_t ip = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, address_size * 2 - 1 + i);
        ip = (ip << 8) | byte;
    }

    *cpu = (struct Cpu) {
        .type = yot_type,
        .ip = ip,
        .dsp = dsp,
        .asp = asp,
        .break_flag = false
    };
}

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

    case 0x02: // Push address literal
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
        uint64_t old_address = MASKED(cpu->ip + 1);
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
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            write(address_bus, MASKED(cpu->asp - address_size + i), b_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2 + i), a_byte);
        }
        INCREMENT_IP;
        break;

    case 0x40: { // Push next instruction pointer
        uint64_t next_ip = MASKED(cpu->ip + 1);
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t next_ip_byte = next_ip && 0xff;
            next_ip >>= 8;
            write(address_bus, cpu->asp, next_ip_byte);
            cpu->asp = MASKED(cpu->asp + 1);
        }
        INCREMENT_IP;
    } break;

    case 0x41: // Address stack equal to zero
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t byte = read(address_bus, MASKED(cpu->asp - 1 - i));
            if (byte != 0) {
                cpu->asp = MASKED(cpu->asp - address_size);
                write(address_bus, cpu->dsp, 0x00);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                break; // TODO
            }
        }
        cpu->asp = MASKED(cpu->asp - address_size);
        write(address_bus, cpu->dsp, 0x01);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        break;

    case 0x42: // Over address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            write(address_bus, cpu->asp, byte);
            cpu->asp = MASKED(cpu->asp + 1);
        }
        INCREMENT_IP;
        break;

    case 0x50: { // Push data stack pointer
        uint64_t dsp = cpu->dsp;
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t dsp_byte = dsp && 0xff;
            dsp >>= 8;
            write(address_bus, cpu->asp, dsp_byte);
            cpu->asp = MASKED(cpu->asp + 1);
        }
        INCREMENT_IP;
    } break;

    case 0x51: // Address stack not equal to zero
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t byte = read(address_bus, MASKED(cpu->asp - 1 - i));
            if (byte != 0) {
                cpu->asp = MASKED(cpu->asp - address_size);
                write(address_bus, cpu->dsp, 0x01);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                break; // TODO
            }
        }
        cpu->asp = MASKED(cpu->asp - address_size);
        write(address_bus, cpu->dsp, 0x00);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        break;

    case 0x52: // Rotate forward address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 3 + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            uint8_t c_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            write(address_bus, MASKED(cpu->asp - address_size * 3 + i), c_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2 + i), a_byte);
            write(address_bus, MASKED(cpu->asp - address_size + i), b_byte);
        }
        INCREMENT_IP;
        break;

    case 0x60: // Set interrupt disable
        cpu->interrupt_disable_flag = true;
        INCREMENT_IP;
        break;

    case 0x61: // Address stack greater than
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            if (a_byte > b_byte) {
                cpu->asp = MASKED(cpu->asp - address_size * 2);
                write(address_bus, cpu->dsp, 0x01);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                break; // TODO
            }
        }
        cpu->asp = MASKED(cpu->asp - address_size * 2);
        write(address_bus, cpu->dsp, 0x00);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        break;

    case 0x62: // Rotate backward address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 3));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2));
            uint8_t c_byte = read(address_bus, MASKED(cpu->asp - address_size));
            write(address_bus, MASKED(cpu->asp - address_size * 3), b_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2), c_byte);
            write(address_bus, MASKED(cpu->asp - address_size), a_byte);
        }
        INCREMENT_IP;
        break;

    case 0x70: // Clear interrupt disable
        cpu->interrupt_disable_flag = false;
        break;

    case 0x71: // Address stack lesser than
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            if (a_byte < b_byte) { 
                cpu->asp = MASKED(cpu->asp - address_size * 2);
                write(address_bus, cpu->dsp, 0x01);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                break; // TODO
            }
        }
        cpu->asp = MASKED(cpu->asp - address_size * 2);
        write(address_bus, cpu->dsp, 0x00);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        break;

    case 0x72: // Extension
        INCREMENT_IP;
        break;

    default: {
        uint8_t instruction_group = (uint8_t) (instruction_type / UINT8_C(0x40) * UINT8_C(0x40)) | (instruction_type & UINT8_C(0x0f));
        size_t instruction_size = (size_t) 1 << (instruction_type / 0x10);
        
        switch (instruction_group) {
        case 0x03: // Push data literal
            for (size_t i = 0; i < instruction_size; ++i) {
                INCREMENT_IP;
                uint8_t literal = read(address_bus, cpu->ip);
                write(address_bus, cpu->dsp, literal);
                cpu->dsp = MASKED(cpu->dsp + 1);
            }
            INCREMENT_IP;
            break;

        case 0x43: // Stash
            for (size_t i = 0; i < address_size - instruction_size; ++i) {
                write(address_bus, cpu->asp, 0x00);
                cpu->asp = MASKED(cpu->asp + 1);
            }
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t literal = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, cpu->asp, literal);
                cpu->asp = MASKED(cpu->asp + 1);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            break;

        case 0x04: // Drop data stack
            cpu->dsp = MASKED(cpu->asp - instruction_size);
            INCREMENT_IP;
            break;

        case 0x44: // Duplicate data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - instruction_size));
                write(address_bus, cpu->dsp, byte);
                cpu->dsp = MASKED(cpu->dsp + 1);
            }
            INCREMENT_IP;
            break;
        
        case 0x05: // Swap data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), b_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i), a_byte);
            }
            INCREMENT_IP;
            break;

        case 0x45: // Over data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                write(address_bus, cpu->dsp, byte);
                cpu->dsp = MASKED(cpu->dsp + 1);
            }
            INCREMENT_IP;
            break;

        case 0x06: // Rotate forward data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                uint8_t c_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i), c_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i), a_byte);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), b_byte);
            }
            INCREMENT_IP;
            break;

        case 0x46: // Rotate backward data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2));
                uint8_t c_byte = read(address_bus, MASKED(cpu->dsp - instruction_size));
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3), b_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2), c_byte);
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte);
            }
            INCREMENT_IP;
            break;

        case 0x07: { // Fetch memory
            uint64_t address = 0;
            for (size_t i = 0; i < address_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->asp - address_size + i));
                address = (address << 8) | byte;
            }
            cpu->asp = MASKED(cpu->asp - address_size);
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(address + i));
                write(address_bus, cpu->dsp, byte);
                cpu->dsp = MASKED(cpu->dsp + 1);
            }
            INCREMENT_IP;
        } break;

        case 0x47: { // Store memory
            uint64_t address = 0;
            for (size_t i = 0; i < address_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->asp - address_size + i));
                address = (address << 8) | byte;
            }
            cpu->asp = MASKED(cpu->asp - address_size);
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(address + i), byte);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
        } break;

        case 0x08: { // Increment data stack
            uint8_t carry = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                int byte = (int) read(address_bus, MASKED(cpu->dsp - i));
                int incremented = byte + 1 + carry;
                carry = ((incremented & 0x100) >> 8);
                uint8_t output_byte = (uint8_t) (incremented & 0xff);
                write(address_bus, MASKED(cpu->dsp - i), output_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x48: { // Decrement data stack
            uint8_t borrow = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                int byte = (int) read(address_bus, MASKED(cpu->dsp - i));
                int decremented = byte - 1 - borrow;
                borrow = ((decremented & 0x100) >> 8);
                uint8_t output_byte = (uint8_t) (decremented & 0xff);
                write(address_bus, MASKED(cpu->dsp - i), output_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x09: { // Add data stack
            uint8_t carry = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                int b_byte = (int) read(address_bus, cpu->dsp);
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size));
                int sum = a_byte + b_byte + carry;
                carry = ((sum & 0x100) >> 8);
                uint8_t c_byte = (uint8_t) (sum & 0xff);
                write(address_bus, MASKED(cpu->dsp - instruction_size), c_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x49: { // Subtract data stack
            uint8_t borrow = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                int b_byte = (int) read(address_bus, cpu->dsp);
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size));
                int difference = a_byte - b_byte - borrow;
                borrow = ((difference & 0x100) >> 8);
                uint8_t c_byte = (uint8_t) (difference & 0xff);
                write(address_bus, MASKED(cpu->dsp - instruction_size), c_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x0a: { // Add with carry data stack
            // a b add -> c
            // 0 a b adc -> c 1 -> c 1 a b adc -> c d 2 -> ...
            // a3 b3 a2 b2 a1 b1 a0 b0 0 adc -> .. a1 b1 1
            // 12345678 -> 15263748
            uint8_t carry = read(address_bus, MASKED(cpu->dsp - 1));
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                int b_byte = (int) read(address_bus, cpu->dsp);
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size));
                int sum = a_byte + b_byte + carry;
                carry = ((sum & 0x100) >> 8);
                uint8_t c_byte = (uint8_t) (sum & 0xff);
                write(address_bus, MASKED(cpu->dsp - instruction_size), c_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x4a: { // Subtract with borrow data stack
            uint8_t borrow = read(address_bus, MASKED(cpu->dsp - 1));
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                int b_byte = (int) read(address_bus, cpu->dsp);
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size));
                int difference = a_byte - b_byte - borrow;
                borrow = ((difference & 0x100) >> 8);
                uint8_t c_byte = (uint8_t) (difference & 0xff);
                write(address_bus, MASKED(cpu->dsp - instruction_size), c_byte);
            }
            INCREMENT_IP;
        } break;

        case 0x0b: // Bitwise not
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), (uint8_t) ~byte);
            }
            INCREMENT_IP;
            break;

        case 0x4b: // And
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                uint8_t a_byte = read(address_bus, cpu->dsp);
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size));
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte & b_byte);
            }
            INCREMENT_IP;
            break;

        case 0x0c: // Inclusive or
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                uint8_t a_byte = read(address_bus, cpu->dsp);
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size));
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte | b_byte);
            }
            INCREMENT_IP;
            break;
        
        case 0x4c: // Exclusive or
            for (size_t i = 0; i < instruction_size; ++i) {
                cpu->dsp = MASKED(cpu->dsp - 1);
                uint8_t a_byte = read(address_bus, cpu->dsp);
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size));
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte ^ b_byte);
            }
            INCREMENT_IP;
            break;
        
        case 0x0d: { // Shift left
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - 1 - i));
                value = (uint16_t) (value << 1);
                shifted_value = (uint8_t) ((value & 0xff00) >> 8);
                write(address_bus, MASKED(cpu->dsp - 1 - i), (uint8_t) (value & 0x00ff) | shifted_value);
            }
            INCREMENT_IP;
        } break;

        case 0x4d: { // Shift right
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                value = (uint16_t) (value << (8 - 1));
                shifted_value = (uint8_t) (value & 0x00ff);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), ((uint8_t) ((value & 0xff00) >> 8)) | shifted_value);
            }
            INCREMENT_IP;
        } break;

        case 0x0e: // Data stack equal to zero
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - 1 - i));
                if (byte != 0) {
                    cpu->dsp = MASKED(cpu->dsp - instruction_size);
                    write(address_bus, cpu->dsp, 0x00);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    break; // TODO
                }
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, 0x01);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            break;

        case 0x4e: // Data stack not equal to zero
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - 1 - i));
                if (byte != 0) {
                    cpu->dsp = MASKED(cpu->dsp - instruction_size);
                    write(address_bus, cpu->dsp, 0x01);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    break; // TODO
                }
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, 0x00);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            break;

        case 0x0f: // Data stack greater than
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                if (a_byte > b_byte) {
                    cpu->dsp = MASKED(cpu->dsp - (uint64_t) instruction_size * 2);
                    write(address_bus, cpu->dsp, 0x01);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    break; // TODO
                }
            }
            cpu->dsp = MASKED(cpu->dsp - (uint64_t) instruction_size * 2);
            write(address_bus, cpu->dsp, 0x00);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            break;

        case 0x4f: // Data stack lesser than
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                if (a_byte < b_byte) {
                    cpu->dsp = MASKED(cpu->dsp - (uint64_t) instruction_size * 2);
                    write(address_bus, cpu->dsp, 0x01);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    break; // TODO
                }
            }
            cpu->dsp = MASKED(cpu->dsp - (uint64_t) instruction_size * 2);
            write(address_bus, cpu->dsp, 0x00);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            break;
        }
    } break;
    }
}
