#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

#include "address_bus.h"

struct Cpu {
    uint64_t mask;
    uint64_t ip;
    uint64_t sp;
};

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus);

#endif // CPU_H_
