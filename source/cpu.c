#include "../include/cpu.h"
#include "string.h"
#include "stdio.h"

BUS CreateMemory(){
    BUS temp;
    memset(temp.mem, 0, 0x10000);
    // Little endian.
    temp.mem[0xFFFC] = 00;
    temp.mem[0xFFFD] = 04;
    return temp;
}

byte readAddress(BUS* bus, word address){
    return bus->mem[address];
}

void writeAddress(BUS* bus, word address, byte data){
    bus->mem[address] = data;
    return;
}

CPU Reset(BUS* bus){
    CPU tempCPU = {0};
    tempCPU.bus = bus;
    tempCPU.SP = 0xFD; // Ghost/Dummy 3 pushes for some reason.
    tempCPU.PC = (readAddress(bus, 0xFFFC) | (((word)readAddress(bus, 0xFFFD)) << 8));
    tempCPU.SR = (FB_IGNORED | FB_IRQD);
    tempCPU.cycles = 0;
    return tempCPU;
}

void PrintCPUState(CPU* cpu){
    fprintf(stderr, "REG A: %x\n", cpu->A);
    fprintf(stderr, "REG X: %x\n", cpu->X);
    fprintf(stderr, "REG Y: %x\n", cpu->Y);
    fprintf(stderr, "REG SP: %x\n", cpu->SP);
    fprintf(stderr, "REG SR: %x\n", cpu->SR);
    fprintf(stderr, "REG PC: %x\n", cpu->PC);
    return;
}

byte AM_IMM(CPU* cpu){
    return cpu->bus->mem[cpu->PC++];
};

byte AM_ABS(CPU* cpu){ // Inling would work, however, C compiler doesn't really promise that if I do PC++ twice in the same line, it'll do it in the same order.
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = (word)(hi << 8)| lo;
    return cpu->bus->mem[addr];
};

byte AM_ZPA(CPU* cpu){
    return cpu->bus->mem[(word)(cpu->bus->mem[cpu->PC++])]; // Don't really need this implicit conversion tbf.
};

typedef enum{
    REG_X,
    REG_Y
}XY;

byte AM_INDEXED(CPU* cpu, XY reg){
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = (word)(hi << 8)| lo;
    word immr = (word)cpu->bus->mem[addr];
    byte HB = (byte)(immr >> 8);
    if(reg == REG_X)
        immr += cpu->X;
    else
        immr += cpu->Y;
    byte newHB = (byte)(immr >> 8);
    if(HB != newHB)
        cpu->cycles++; // Page crossing.
    return cpu->bus->mem[immr];
}

byte AM_INDEXED_ZPA(CPU* cpu, XY reg){
    byte immr = cpu->bus->mem[(word)(cpu->bus->mem[cpu->PC++])];
    if(reg == REG_X)
        immr += cpu->X;
    else
        immr += cpu->Y;
    return cpu->bus->mem[(word)immr]; // un-needed but just for the sake of clarity.
}

word AM_INDIRECT(CPU* cpu){
    byte prevhi = (byte)((cpu->PC) >> 8);
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = (word)(hi << 8)| lo;
    // Original NMOS 'quirk'
    byte targetLO = cpu->bus->mem[addr];
    byte targetHO;
    if(lo == 0xFF)
        targetHO = cpu->bus->mem[(word)hi << 8];
    else
        targetHO = cpu->bus->mem[addr + 1];
    return (word)targetHO << 8 | targetLO;
}

byte AM_PREINDEXED_INDIRECT(CPU* cpu){
    byte immr = cpu->bus->mem[(word)(cpu->bus->mem[cpu->PC++])];
    immr += cpu->X;
    word lookup = cpu->bus->mem[cpu->bus->mem[immr] | ((word)(cpu->bus->mem[immr + 1]) << 8)];
    return cpu->bus->mem[lookup];
}

byte AM_POSTIDNEXED_INDIRECT(CPU* cpu){
    byte immr = cpu->bus->mem[(word)(cpu->bus->mem[cpu->PC++])];
    word lookup = cpu->bus->mem[cpu->bus->mem[immr] | ((word)(cpu->bus->mem[immr + 1]) << 8)];
    byte HB = (byte)(lookup >> 8);
    lookup += cpu->Y;
    byte newHB = (byte)(lookup >> 8);
    if(HB != newHB)
        cpu->cycles++; // Page crossing.
    return cpu->bus->mem[lookup];
}

void Step(CPU* cpu){
    while(1){
        PrintCPUState(cpu);
        byte OPCODE = cpu->bus->mem[cpu->PC++];
        switch (OPCODE){
            case 
        }
    }
}

void LDA(CPU* cpu, byte data){
    cpu->A = data;
}

void LDX(CPU* cpu, byte data){
    cpu->X = data;
}

void LDY(CPU* cpu, byte data){
    cpu->Y = data;
}

void STA(CPU* cpu, word address){
    cpu->bus->mem[address] = cpu->A;
}

void TAX(CPU* cpu){
    cpu->X = cpu->A;
}

void TAY(CPU* cpu){
    cpu->Y = cpu->A;
}

void TXA(CPU* cpu){
    cpu->A = cpu->X;
}

void TYA(CPU* cpu){
    cpu->A = cpu->Y;
}

void JMP(CPU* cpu, word address){
    cpu->PC = address;
}

// Since the relative address can be negative, I use i8 instead of the byte, since both the uint16_t and i8_t will get promoted to int32_t and not uint32_t, there should be no errors.
void BEQ(CPU* cpu, i8 relAddr){
    if((cpu->SR & FB_Z) != 0)
        cpu->PC = cpu->PC + relAddr;
}

void BNE(CPU* cpu, i8 relAddr){
    if((cpu->SR & FB_Z) == 0)
        cpu->PC = cpu->PC + relAddr;
}
