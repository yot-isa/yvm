#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

#include "address_bus.h"

enum Yot_Type {
    YOT_8 = 0,
    YOT_16 = 1,
    YOT_32 = 2,
    YOT_64 = 3
};

struct Cpu {
    enum Yot_Type type;
    uint64_t ip;
    uint64_t dsp;
    uint64_t asp;
    bool break_flag;
    bool interrupt_disable_flag;
};

void execute_next_instruction(struct Cpu *cpu, struct Address_Bus *address_bus);

#endif // CPU_H_
