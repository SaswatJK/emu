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

typedef struct{
    word PC; // It shoudl read from thje address provided in the 16-bit reset vector at FFFC.
    byte A;
    byte X; // X and Y are index registers whose content is added to the base memory location, before any values are either stored to or retreived from the "result address".
    byte Y;
    byte SR; // Status register, or also a P register.
    byte SP; // Stack pointer.
}CPU;

typedef struct{
    byte data[0x10000];
}RAM;

RAM CreateMemory();

byte readAddress(RAM* ram, word address);

CPU Reset(RAM* ram);

#endif
