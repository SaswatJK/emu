#ifndef CPU_H
#define CPU_H

#include "utils.h"

/*
    Status Register Flags (bit 7 to bit 0)
    N   Negative
    V   Overflow
    -   ignored   -> Is always 1.
    B   Break
    D   Decimal (use BCD for arithmetics)
    I   Interrupt (IRQ disable)
    Z   Zero
    C   Carry
*/

typedef enum{
    FB_N       = 0b10000000,
    FB_V       = 0b01000000,
    FB_IGNORED = 0b00100000,
    FB_B       = 0b00010000,
    FB_D       = 0b00001000,
    FB_I       = 0b00000100,
    FB_IRQD    = 0b00000100,
    FB_Z       = 0b00000010,
    FB_C       = 0b00000001
}FlagBits; // Will use ~ to complement it for checking. Decided not to make a seperate enum for masks.


typedef struct{
    byte mem[0x10000];
}BUS;

typedef struct{
    u64  cycles;
    BUS* bus;
    word PC; // It shoudl read from thje address provided in the 16-bit reset vector at FFFC.
    byte A;
    byte X; // X and Y are index registers whose content is added to the base memory location, before any values are either stored to or retreived from the "result address".
    byte Y;
    byte SR; // Status register, or also a P register.
    byte SP; // Stack pointer.
}CPU;

BUS CreateMemory();
byte readAddress(BUS* bus, word address);
void writeAddress(BUS* bus, word address, byte data);

CPU  Reset(BUS* bus);
void Step(CPU* cpu);

void LDA(CPU* cpu, byte data);
void LDX(CPU* cpu, byte data);
void LDY(CPU* cpu, byte data);
void STA(CPU* cpu, word address);
void TAX(CPU* cpu);
void TAY(CPU* cpu);
void TXA(CPU* cpu);
void TYA(CPU* cpu);
void JMP(CPU* cpu, word address);
void BEQ(CPU* cpu, i8 relAddr);
void BNE(CPU* cpu, i8 relAddr);

#endif
