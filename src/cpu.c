#include <stdio.h>

#include "cpu.h"

static const char *MNEMONICS[256] = {
    "brk", "lit^", "inc^", "lit1", "fcm1", "drp1", "swp1", "rtf1", "inc1", "eqz1", "add1", "adc1", "bnt1", "ior1", "shl1", "rtl1",
    "nop", "ext", "dec^", "lit2", "fcm2", "drp2", "swp2", "rtf2", "inc2", "eqz2", "add2", "adc2", "bnt2", "ior2", "shl2", "rtl2",
    "jmp", "drp^", "eqz^", "lit4", "fcm4", "drp4", "swp4", "rtf4", "inc4", "eqz4", "add4", "adc4", "bnt4", "ior4", "shl4", "rtl4",
    "jsr", "dup^", "nez^", "lit8", "fcm8", "drp8", "swp8", "rtf8", "inc8", "eqz8", "add8", "adc8", "bnt8", "ior8", "shl8", "rtl8",
    "nip", "swp^", "add^", "sts1", "stm1", "dup1", "ovr1", "rtb1", "dec1", "nez1", "sub1", "sbb1", "and1", "xor1", "shr1", "rtr1",
    "dsp", "ovr^", "sub^", "sts2", "stm2", "dup2", "ovr2", "rtb2", "dec2", "nez2", "sub2", "sbb2", "and2", "xor2", "shr2", "rtr2",
    "sei", "rtf^", "adc^", "sts4", "stm4", "dup4", "ovr4", "rtb4", "dec4", "nez4", "sub4", "sbb4", "and4", "xor4", "shr4", "rtr4",
    "cli", "rtb^", "sbb^", "sts8", "stm8", "dup8", "ovr8", "rtb8", "dec8", "nez8", "sub8", "sbb8", "and8", "xor8", "shr8", "rtr8",
    "brk?", "lit^?", "inc^?", "lit1?", "fcm1?", "drp1?", "swp1?", "rtf1?", "inc1?", "eqz1?", "add1?", "adc1?", "bnt1?", "ior1?", "shl1?", "rtl1?",
    "nop?", "ext?", "dec^?", "lit2?", "fcm2?", "drp2?", "swp2?", "rtf2?", "inc2?", "eqz2?", "add2?", "adc2?", "bnt2?", "ior2?", "shl2?", "rtl2?",
    "jmp?", "drp^?", "eqz^?", "lit4?", "fcm4?", "drp4?", "swp4?", "rtf4?", "inc4?", "eqz4?", "add4?", "adc4?", "bnt4?", "ior4?", "shl4?", "rtl4?",
    "jsr?", "dup^?", "nez^?", "lit8?", "fcm8?", "drp8?", "swp8?", "rtf8?", "inc8?", "eqz8?", "add8?", "adc8?", "bnt8?", "ior8?", "shl8?", "rtl8?",
    "nip?", "swp^?", "add^?", "sts1?", "stm1?", "dup1?", "ovr1?", "rtb1?", "dec1?", "nez1?", "sub1?", "sbb1?", "and1?", "xor1?", "shr1?", "rtr1?",
    "dsp?", "ovr^?", "sub^?", "sts2?", "stm2?", "dup2?", "ovr2?", "rtb2?", "dec2?", "nez2?", "sub2?", "sbb2?", "and2?", "xor2?", "shr2?", "rtr2?",
    "sei?", "rtf^?", "adc^?", "sts4?", "stm4?", "dup4?", "ovr4?", "rtb4?", "dec4?", "nez4?", "sub4?", "sbb4?", "and4?", "xor4?", "shr4?", "rtr4?",
    "cli?", "rtb^?", "sbb^?", "sts8?", "stm8?", "dup8?", "ovr8?", "rtb8?", "dec8?", "nez8?", "sub8?", "sbb8?", "and8?", "xor8?", "shr8?", "rtr8?"
};

void cpu_initialize(struct Cpu *cpu, struct Address_Bus *address_bus, enum Yot_Type yot_type)
{
    size_t address_size = (size_t) 1 << yot_type;

    uint64_t dsp = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, i);
        dsp = (dsp << 8) | byte;
    }
    uint64_t asp = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, address_size + i);
        asp = (asp << 8) | byte;
    }
    uint64_t ip = 0;
    for (size_t i = 0; i < address_size; ++i) {
        uint8_t byte = read(address_bus, address_size * 2 + i);
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

#define MASKED(x) ((x) & (uint64_t[]){ 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff }[cpu->type])
#define INCREMENT_IP cpu->ip = MASKED(cpu->ip + 1)
#define SET_BREAK_FLAG cpu->break_flag = true
#define DROP_ADDRESS cpu->asp = MASKED(cpu->asp - address_size)
#define PUSH_ADDRESS cpu->asp = MASKED(cpu->asp + address_size)

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus)
{
    uint8_t instruction = read(address_bus, cpu->ip);
    uint8_t instruction_type = instruction & 0x7f;
    bool conditional = (bool) ((instruction & 0x80) >> 7);
    size_t address_size = (size_t) 1 << cpu->type;
    printf("inst: %s (%02X)\n", MNEMONICS[instruction], instruction);

    if (conditional) {
        cpu->dsp = MASKED(cpu->dsp - 1);
        uint8_t pred = read(address_bus, cpu->dsp);
        if (pred == 0x00) {
            INCREMENT_IP;
            return;
        }
    }

    switch (instruction_type) {
    case 0x00: // Break
        SET_BREAK_FLAG;
        INCREMENT_IP;
        return;

    case 0x10: // No operation
        INCREMENT_IP;
        return;

    case 0x20: { // Jump
        uint64_t dropped_address = 0;
        for (size_t i = 0; i < address_size; ++i)
            dropped_address = (dropped_address << 8) | read(address_bus, MASKED(cpu->asp - address_size + i));
        DROP_ADDRESS;
        cpu->ip = dropped_address;
    } return;

    case 0x30: { // Jump to subroutine
        uint64_t dropped_address = 0;
        uint64_t pushed_address = MASKED(cpu->ip + 1);
        for (size_t i = 0; i < address_size; ++i) {
            dropped_address = (dropped_address << 8) | read(address_bus, MASKED(cpu->asp - address_size + i));
            write(address_bus, MASKED(cpu->asp - 1 - i), pushed_address & 0xff);
            pushed_address >>= 8;
        }
        cpu->ip = dropped_address;
    } return;

    case 0x40: { // Push next instruction pointer
        uint64_t pushed_address = MASKED(cpu->ip + 1);
        for (size_t i = 0; i < address_size; ++i) {
            write(address_bus, MASKED(cpu->asp - 1 - i), pushed_address & 0xff);
            pushed_address >>= 8;
        }
        PUSH_ADDRESS;
        INCREMENT_IP;
    } return;

    case 0x50: { // Push data stack pointer
        uint64_t pushed_address = cpu->dsp;
        for (size_t i = 0; i < address_size; ++i) {
            write(address_bus, MASKED(cpu->asp - 1 - i), pushed_address & 0xff);
            pushed_address >>= 8;
        }
        PUSH_ADDRESS;
        INCREMENT_IP;
    } return;

    case 0x60: // Set interrupt disable
        cpu->interrupt_disable_flag = true;
        INCREMENT_IP;
        return;

    case 0x70: // Clear interrupt disable
        cpu->interrupt_disable_flag = false;
        INCREMENT_IP;
        return;

    case 0x01: // Push address literal
        for (size_t i = 0; i < address_size; ++i)
            write(address_bus, MASKED(cpu->asp + i), read(address_bus, MASKED(cpu->ip + 1 + i)));
        PUSH_ADDRESS;
        cpu->ip = MASKED(cpu->ip + 1 + address_size);
        return;

    case 0x11: // Extension
        INCREMENT_IP;
        return;
    
    case 0x21: // Drop address stack
        DROP_ADDRESS;
        INCREMENT_IP;
        return;

    case 0x31: // Duplicate address stack
        for (size_t i = 0; i < address_size; ++i)
            write(address_bus, MASKED(cpu->asp + i), read(address_bus, MASKED(cpu->asp - address_size + i)));
        PUSH_ADDRESS;
        INCREMENT_IP;
        return;

    case 0x41: // Swap address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            write(address_bus, MASKED(cpu->asp - address_size + i), b_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2 + i), a_byte);
        }
        INCREMENT_IP;
        return;

    case 0x51: // Over address stack
        for (size_t i = 0; i < address_size; ++i)
            write(address_bus, MASKED(cpu->asp + i), read(address_bus, MASKED(cpu->asp - address_size * 2 + i)));
        PUSH_ADDRESS;
        INCREMENT_IP;
        return;

    case 0x61: // Rotate forward address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 3 + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            uint8_t c_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            write(address_bus, MASKED(cpu->asp - address_size * 3 + i), c_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2 + i), a_byte);
            write(address_bus, MASKED(cpu->asp - address_size + i), b_byte);
        }
        INCREMENT_IP;
        return;

    case 0x71: // Rotate backward address stack
        for (size_t i = 0; i < address_size; ++i) {
            uint8_t a_byte = read(address_bus, MASKED(cpu->asp - address_size * 3 + i));
            uint8_t b_byte = read(address_bus, MASKED(cpu->asp - address_size * 2 + i));
            uint8_t c_byte = read(address_bus, MASKED(cpu->asp - address_size + i));
            write(address_bus, MASKED(cpu->asp - address_size * 3 + i), b_byte);
            write(address_bus, MASKED(cpu->asp - address_size * 2 + i), c_byte);
            write(address_bus, MASKED(cpu->asp - address_size + i), a_byte);
        }
        INCREMENT_IP;
        return;

    case 0x02: { // Increment address stack
        uint8_t carry = 1;
        for (size_t i = 0; i < address_size; ++i) {
            int incremented = (int) read(address_bus, MASKED(cpu->asp - i)) + carry;
            carry = ((incremented & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - i), (uint8_t) (incremented & 0xff));
        }
        INCREMENT_IP;
    } return;

    case 0x12: { // Decrement address stack
        uint8_t borrow = 1;
        for (size_t i = 0; i < address_size; ++i) {
            int decremented = (int) read(address_bus, MASKED(cpu->asp - i)) - borrow;
            borrow = ((decremented & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - i), (uint8_t) (decremented & 0xff));
        }
        INCREMENT_IP;
    } return;

    case 0x22: // Address stack equal to zero
        for (size_t i = 0; i < address_size; ++i) {
            if (read(address_bus, MASKED(cpu->asp - 1 - i)) != 0) {
                DROP_ADDRESS;
                write(address_bus, cpu->dsp, 0x00);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                return;
            }
        }
        DROP_ADDRESS;
        write(address_bus, cpu->dsp, 0x01);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        return;

    case 0x32: // Address stack not equal to zero
        for (size_t i = 0; i < address_size; ++i) {
            if (read(address_bus, MASKED(cpu->asp - 1 - i)) != 0) {
                DROP_ADDRESS;
                write(address_bus, cpu->dsp, 0x01);
                cpu->dsp = MASKED(cpu->dsp + 1);
                INCREMENT_IP;
                return;
            }
        }
        DROP_ADDRESS;
        write(address_bus, cpu->dsp, 0x00);
        cpu->dsp = MASKED(cpu->dsp + 1);
        INCREMENT_IP;
        return;

    case 0x42: { // Add address stack
        uint8_t carry = 0;
        for (size_t i = 0; i < address_size; ++i) {
            int b_byte = (int) read(address_bus, MASKED(cpu->asp - 1 - i));
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size - 1 - i));
            int sum = a_byte + b_byte + carry;
            carry = ((sum & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - address_size - 1 - i), (uint8_t) (sum & 0xff));
        }
        DROP_ADDRESS;
        INCREMENT_IP;
    } return;
    
    case 0x52: { // Subtract address stack
        uint8_t borrow = 0;
        for (size_t i = 0; i < address_size; ++i) {
            int b_byte = (int) read(address_bus, MASKED(cpu->asp - 1 - i));
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size - 1 - i));
            int difference = a_byte - b_byte - borrow;
            borrow = ((difference & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - address_size - 1 - i), (uint8_t) (difference & 0xff));
        }
        DROP_ADDRESS;
        INCREMENT_IP;
    } return;

    case 0x62: { // Add with carry address stack
        uint8_t carry = read(address_bus, MASKED(cpu->dsp - 1)) ? 0x01 : 0x00;
        for (size_t i = 0; i < address_size; ++i) {
            int b_byte = (int) read(address_bus, MASKED(cpu->asp - 1 - i));
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size - 1 - i));
            int sum = a_byte + b_byte + carry;
            carry = ((sum & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - address_size - 1 - i), (uint8_t) (sum & 0xff));
        }
        DROP_ADDRESS;
        write(address_bus, MASKED(cpu->dsp - 1), carry);
        INCREMENT_IP;
    } return;
    
    case 0x72: { // Subtract with borrow address stack
        uint8_t borrow = read(address_bus, MASKED(cpu->dsp - 1)) ? 0x01 : 0x00;
        for (size_t i = 0; i < address_size; ++i) {
            int b_byte = (int) read(address_bus, MASKED(cpu->asp - 1 - i));
            int a_byte = (int) read(address_bus, MASKED(cpu->asp - address_size - 1 - i));
            int difference = a_byte - b_byte - borrow;
            borrow = ((difference & 0x100) >> 8);
            write(address_bus, MASKED(cpu->asp - address_size - 1 - i), (uint8_t) (difference & 0xff));
        }
        DROP_ADDRESS;
        write(address_bus, MASKED(cpu->dsp - 1), borrow);
        INCREMENT_IP;
    } return;
    
    default: {
        uint8_t instruction_group = (uint8_t) (instruction_type / 0x40 * 0x40) | (instruction_type & 0x0f);
        size_t instruction_size = (size_t) (1 << (instruction_type / 0x10 % 4));

        switch (instruction_group) {
        case 0x03: // Push data literal
            for (size_t i = 0; i < instruction_size; ++i)
                write(address_bus, MASKED(cpu->dsp + i), read(address_bus, MASKED(cpu->ip + 1 + i)));
            cpu->dsp = MASKED(cpu->dsp + instruction_size);
            cpu->ip = MASKED(cpu->ip + 1 + instruction_size);
            return;

        case 0x43: // Stash
            for (size_t i = 0; i < address_size - instruction_size; ++i) {
                write(address_bus, cpu->asp, 0x00);
                cpu->asp = MASKED(cpu->asp + 1);
            }
            for (size_t i = 0; i < instruction_size; ++i) {
                write(address_bus, cpu->asp, read(address_bus, MASKED(cpu->dsp - instruction_size + i)));
                cpu->asp = MASKED(cpu->asp + 1);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            return;

        case 0x04: { // Fetch memory
            uint64_t address = 0;
            for (size_t i = 0; i < address_size; ++i)
                address = (address << 8) | read(address_bus, MASKED(cpu->asp - address_size + i));
            DROP_ADDRESS;
            for (size_t i = 0; i < instruction_size; ++i)
                write(address_bus, MASKED(cpu->dsp + i), read(address_bus, MASKED(address + i)));
            cpu->dsp = MASKED(cpu->dsp + instruction_size);
            INCREMENT_IP;
        } return;

        case 0x44: { // Store memory
            uint64_t address = 0;
            for (size_t i = 0; i < address_size; ++i)
                address = (address << 8) | read(address_bus, MASKED(cpu->asp - address_size + i));
            DROP_ADDRESS;
            for (size_t i = 0; i < instruction_size; ++i)
                write(address_bus, MASKED(address + i), read(address_bus, MASKED(cpu->dsp - instruction_size + i)));
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
        } return;

        case 0x05: // Drop data stack
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            return;

        case 0x45: // Duplicate data stack
            for (size_t i = 0; i < instruction_size; ++i)
                write(address_bus, MASKED(cpu->dsp + i), read(address_bus, MASKED(cpu->dsp - instruction_size + i)));
            cpu->dsp = MASKED(cpu->dsp + instruction_size);
            INCREMENT_IP;
            return;
        
        case 0x06: // Swap data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), b_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i), a_byte);
            }
            INCREMENT_IP;
            return;

        case 0x46: // Over data stack
            for (size_t i = 0; i < instruction_size; ++i)
                write(address_bus, MASKED(cpu->dsp + i), read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i)));
            cpu->dsp = MASKED(cpu->dsp + instruction_size);
            INCREMENT_IP;
            return;

        case 0x07: // Rotate forward data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                uint8_t c_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i), c_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i), a_byte);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), b_byte);
            }
            INCREMENT_IP;
            return;

        case 0x47: // Rotate backward data stack
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i));
                uint8_t c_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 3 + i), b_byte);
                write(address_bus, MASKED(cpu->dsp - (uint64_t) instruction_size * 2 + i), c_byte);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), a_byte);
            }
            INCREMENT_IP;
            return;

        case 0x08: { // Increment data stack
            uint8_t carry = 1;
            for (size_t i = 0; i < instruction_size; ++i) {
                int incremented = (int) read(address_bus, MASKED(cpu->dsp - 1 - i)) + carry;
                carry = ((incremented & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - 1 - i), (uint8_t) (incremented & 0xff));
            }
            INCREMENT_IP;
        } return;

        case 0x48: { // Decrement data stack
            // 00 00 00 06 -> FE FE FF 05
            uint8_t borrow = 1;
            for (size_t i = 0; i < instruction_size; ++i) {
                int decremented = (int) read(address_bus, MASKED(cpu->dsp - 1 - i)) - borrow;
                borrow = ((decremented & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - 1 - i), (uint8_t) (decremented & 0xff));
            }
            INCREMENT_IP;
        } return;

        case 0x09: // Data stack equal to zero
            for (size_t i = 0; i < instruction_size; ++i) {
                if (read(address_bus, MASKED(cpu->dsp - 1 - i)) != 0) {
                    cpu->dsp = MASKED(cpu->dsp - instruction_size);
                    write(address_bus, cpu->dsp, 0x00);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    return;
                }
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, 0x01);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            return;

        case 0x49: // Data stack not equal to zero
            for (size_t i = 0; i < instruction_size; ++i) {
                if (read(address_bus, MASKED(cpu->dsp - 1 - i)) != 0) {
                    cpu->dsp = MASKED(cpu->dsp - instruction_size);
                    write(address_bus, cpu->dsp, 0x01);
                    cpu->dsp = MASKED(cpu->dsp + 1);
                    INCREMENT_IP;
                    return;
                }
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, 0x00);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
            return;

        case 0x0a: { // Add data stack
            uint8_t carry = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                int b_byte = (int) read(address_bus, MASKED(cpu->dsp - 1 - i));
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size - 1 - i));
                int sum = a_byte + b_byte + carry;
                carry = ((sum & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - instruction_size - 1 - i), (uint8_t) (sum & 0xff));
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
        } return;

        case 0x4a: { // Subtract data stack
            uint8_t borrow = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                int b_byte = (int) read(address_bus, MASKED(cpu->dsp - 1 - i));
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size - 1 - i));
                int difference = a_byte - b_byte - borrow;
                borrow = ((difference & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - instruction_size), (uint8_t) (difference & 0xff));
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
        } return;

        case 0x0b: { // Add with carry data stack
            uint8_t carry = read(address_bus, MASKED(cpu->dsp - 1)) ? 0x01 : 0x00;
            cpu->dsp = MASKED(cpu->dsp - 1);
            for (size_t i = 0; i < instruction_size; ++i) {
                int b_byte = (int) read(address_bus, MASKED(cpu->dsp - 1 - i));
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size - 1 - i));
                int sum = a_byte + b_byte + carry;
                carry = ((sum & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - instruction_size), (uint8_t) (sum & 0xff));
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, carry);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
        } return;

        case 0x4b: { // Subtract with borrow data stack
            uint8_t borrow = read(address_bus, MASKED(cpu->dsp - 1)) ? 0x01 : 0x00;
            cpu->dsp = MASKED(cpu->dsp - 1);
            for (size_t i = 0; i < instruction_size; ++i) {
                int b_byte = (int) read(address_bus, MASKED(cpu->dsp - 1 - i));
                int a_byte = (int) read(address_bus, MASKED(cpu->dsp - instruction_size - 1 - i));
                int difference = a_byte - b_byte - borrow;
                borrow = ((difference & 0x100) >> 8);
                write(address_bus, MASKED(cpu->dsp - instruction_size), (uint8_t) (difference & 0xff));
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            write(address_bus, cpu->dsp, borrow);
            cpu->dsp = MASKED(cpu->dsp + 1);
            INCREMENT_IP;
        } return;

        case 0x0c: // Bitwise not
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), (uint8_t) ~byte);
            }
            INCREMENT_IP;
            return;

        case 0x4c: // And
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size * 2 + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size * 2 + i), a_byte & b_byte);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            return;

        case 0x0d: // Inclusive or
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size * 2 + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte | b_byte);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            return;
        
        case 0x4d: // Exclusive or
            for (size_t i = 0; i < instruction_size; ++i) {
                uint8_t a_byte = read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                uint8_t b_byte = read(address_bus, MASKED(cpu->dsp - instruction_size * 2 + i));
                write(address_bus, MASKED(cpu->dsp - instruction_size), a_byte ^ b_byte);
            }
            cpu->dsp = MASKED(cpu->dsp - instruction_size);
            INCREMENT_IP;
            return;
        
        case 0x0e: { // Shift left
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - 1 - i));
                value = (uint16_t) (value << 1);
                shifted_value = (uint8_t) ((value & 0xff00) >> 8);
                write(address_bus, MASKED(cpu->dsp - 1 - i), (uint8_t) (value & 0x00ff) | shifted_value);
            }
            INCREMENT_IP;
        } return;

        case 0x4e: { // Shift right
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                value = (uint16_t) (value << (8 - 1));
                shifted_value = (uint8_t) (value & 0x00ff);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), ((uint8_t) ((value & 0xff00) >> 8)) | shifted_value);
            }
            INCREMENT_IP;
        } return;

        case 0x0f: { // Rotate left
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - 1 - i));
                value = (uint16_t) (value << 1);
                shifted_value = (uint8_t) ((value & 0xff00) >> 8);
                write(address_bus, MASKED(cpu->dsp - 1 - i), (uint8_t) (value & 0x00ff) | shifted_value);
            }
            INCREMENT_IP;
        } return;

        case 0x4f: { // Rotate right
            uint8_t shifted_value = 0;
            for (size_t i = 0; i < instruction_size; ++i) {
                uint16_t value = (uint16_t) read(address_bus, MASKED(cpu->dsp - instruction_size + i));
                value = (uint16_t) (value << (8 - 1));
                shifted_value = (uint8_t) (value & 0x00ff);
                write(address_bus, MASKED(cpu->dsp - instruction_size + i), ((uint8_t) ((value & 0xff00) >> 8)) | shifted_value);
            }
            INCREMENT_IP;
        } return;
        }
    } return;
    }
}
