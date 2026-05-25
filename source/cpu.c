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

#define AM_REL(CPUpointer) AM_IMM((CPUpointer))

word AM_ABS(CPU* cpu){ // Inling would work, however, C compiler doesn't really promise that if I do PC++ twice in the same line, it'll do it in the same order.
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = ((word)hi << 8)| lo;
    return addr;
};

word AM_ZPA(CPU* cpu){
    return (word)(cpu->bus->mem[cpu->PC++]); // Don't really need this implicit conversion tbf.
};

typedef enum{
    REG_X,
    REG_Y
}XY;

word AM_ABS_INDEXED(CPU* cpu, XY reg, _Bool PAGE_CROSS){
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = ((word)hi << 8)| lo;
    byte HB = (byte)(addr >> 8);
    if(reg == REG_X)
        addr += cpu->X;
    else
        addr += cpu->Y;
    byte newHB = (byte)(addr >> 8);
    if(HB != newHB)
        if(PAGE_CROSS)
            cpu->cycles++; // Page crossing.
    return addr;
}

word AM_INDEXED_ZPA(CPU* cpu, XY reg){
    byte immr = cpu->bus->mem[cpu->PC++];
    if(reg == REG_X)
        immr += cpu->X;
    else
        immr += cpu->Y;
    return (word)immr; // un-needed but just for the sake of clarity.
}

word AM_INDIRECT(CPU* cpu){
    byte lo = (word)cpu->bus->mem[cpu->PC++];
    byte hi = (word)(cpu->bus->mem[cpu->PC++]);
    word addr = ((word)hi << 8)| lo;
    byte targetLO = cpu->bus->mem[addr];
    // Original NMOS 'quirk'
    byte targetHO = cpu->bus->mem[(addr & 0xFF00) | (0x00FF & (addr + 1))];
    return ((word)targetHO << 8) | targetLO;
}

word AM_PREINDEXED_INDIRECT(CPU* cpu){
    byte immr = cpu->bus->mem[cpu->PC++];
    immr += cpu->X;
    word lookup = cpu->bus->mem[immr] | ((word)(cpu->bus->mem[immr + 1]) << 8);
    return lookup;
}

word AM_POSTIDNEXED_INDIRECT(CPU* cpu, _Bool PAGE_CROSS){
    byte immr = cpu->bus->mem[cpu->PC++];
    word lookup = cpu->bus->mem[immr] | ((word)(cpu->bus->mem[immr + 1]) << 8);
    byte HB = (byte)(lookup >> 8);
    lookup += cpu->Y;
    byte newHB = (byte)(lookup >> 8);
    if(HB != newHB)
        if(PAGE_CROSS)
            cpu->cycles++; // Page crossing.
    return lookup;
}

void Step(CPU* cpu){
    #ifdef DEBUG
        PrintCPUState(cpu);
    #endif
    byte OPCODE = cpu->bus->mem[cpu->PC++];
    //fprintf(stderr, "OPCODE: %x\n", OPCODE);
    switch(OPCODE){
        case 0x00: {
            BRK(cpu); cpu->cycles += 7; break; }
        case 0x01: {
            ORA(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0x05: {
            ORA(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0x06: {
            ASL_MEM(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0x08: {
            PHP(cpu); cpu->cycles += 3; break; }
        case 0x09: {
            ORA(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0x0A: {
            ASL_ACC(cpu); cpu->cycles += 2; break; }
        case 0x0D: {
            ORA(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0x0E: {
            ASL_MEM(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0x10: {
            BPL(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0x11: {
            ORA(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0x15: {
            ORA(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0x16: {
            ASL_MEM(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0x18: {
            CLC(cpu); cpu->cycles += 2; break; }
        case 0x19: {
            ORA(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0x1D: {
            ORA(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0x1E: {
            ASL_MEM(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        case 0x20: {
            JSR(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0x21: {
            AND(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0x24: {
            BIT(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0x25: {
            AND(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0x26: {
            ROL_MEM(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0x28: {
            PLP(cpu); cpu->cycles += 4; break; }
        case 0x29: {
            AND(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0x2A: {
            ROL_ACC(cpu); cpu->cycles += 2; break; }
        case 0x2C: {
            BIT(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0x2D: {
            AND(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0x2E: {
            ROL_MEM(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0x30: {
            BMI(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0x31: {
            AND(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0x35: {
            AND(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0x36: {
            ROL_MEM(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0x38: {
            SEC(cpu); cpu->cycles += 2; break; }
        case 0x39: {
            AND(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0x3D: {
            AND(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0x3E: {
            ROL_MEM(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        case 0x40: {
            RTI(cpu); cpu->cycles += 6; break; }
        case 0x41: {
            EOR(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0x45: {
            EOR(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0x46: {
            LSR_MEM(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0x48: {
            PHA(cpu); cpu->cycles += 3; break; }
        case 0x49: {
            EOR(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0x4A: {
            LSR_ACC(cpu); cpu->cycles += 2; break; }
        case 0x4C: {
            JMP(cpu, AM_ABS(cpu)); cpu->cycles += 3; break; }
        case 0x4D: {
            EOR(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0x4E: {
            LSR_MEM(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0x50: {
            BVC(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0x51: {
            EOR(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0x55: {
            EOR(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0x56: {
            LSR_MEM(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0x58: {
            CLI(cpu); cpu->cycles += 2; break; }
        case 0x59: {
            EOR(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0x5D: {
            EOR(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0x5E: {
            LSR_MEM(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        case 0x60: {
            RTS(cpu); cpu->cycles += 6; break; }
        case 0x61: {
            ADC(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0x65: {
            ADC(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0x66: {
            ROR_MEM(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0x68: {
            PLA(cpu); cpu->cycles += 4; break; }
        case 0x69: {
            ADC(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0x6A: {
            ROR_ACC(cpu); cpu->cycles += 2; break; }
        case 0x6C: {
            JMP(cpu, AM_INDIRECT(cpu)); cpu->cycles += 5; break; }
        case 0x6D: {
            ADC(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0x6E: {
            ROR_MEM(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0x70: {
            BVS(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0x71: {
            ADC(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0x75: {
            ADC(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0x76: {
            ROR_MEM(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0x78: {
            SEI(cpu); cpu->cycles += 2; break; }
        case 0x79: {
            ADC(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0x7D: {
            ADC(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0x7E: {
            ROR_MEM(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        case 0x81: {
            STA(cpu, AM_PREINDEXED_INDIRECT(cpu)); cpu->cycles += 6; break; }
        case 0x84: {
            STY(cpu, AM_ZPA(cpu)); cpu->cycles += 3; break; }
        case 0x85: {
            STA(cpu, AM_ZPA(cpu)); cpu->cycles += 3; break; }
        case 0x86: {
            STX(cpu, AM_ZPA(cpu)); cpu->cycles += 3; break; }
        case 0x88: {
            DEY(cpu); cpu->cycles += 2; break; }
        case 0x8A: {
            TXA(cpu); cpu->cycles += 2; break; }
        case 0x8C: {
            STY(cpu, AM_ABS(cpu)); cpu->cycles += 4; break; }
        case 0x8D: {
            STA(cpu, AM_ABS(cpu)); cpu->cycles += 4; break; }
        case 0x8E: {
            STX(cpu, AM_ABS(cpu)); cpu->cycles += 4; break; }
        case 0x90: {
            BCC(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0x91: {
            STA(cpu, AM_POSTIDNEXED_INDIRECT(cpu, 0)); cpu->cycles += 6; break; }
        case 0x94: {
            STY(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 4; break; }
        case 0x95: {
            STA(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 4; break; }
        case 0x96: {
            STX(cpu, AM_INDEXED_ZPA(cpu, REG_Y)); cpu->cycles += 4; break; }
        case 0x98: {
            TYA(cpu); cpu->cycles += 2; break; }
        case 0x99: {
            STA(cpu, AM_ABS_INDEXED(cpu, REG_Y, 0)); cpu->cycles += 5; break; }
        case 0x9A: {
            TXS(cpu); cpu->cycles += 2; break; }
        case 0x9D: {
            STA(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 5; break; }
        case 0xA0: {
            LDY(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xA1: {
            LDA(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0xA2: {
            LDX(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xA4: {
            LDY(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xA5: {
            LDA(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xA6: {
            LDX(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xA8: {
            TAY(cpu); cpu->cycles += 2; break; }
        case 0xA9: {
            LDA(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xAA: {
            TAX(cpu); cpu->cycles += 2; break; }
        case 0xAC: {
            LDY(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xAD: {
            LDA(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xAE: {
            LDX(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xB0: {
            BCS(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0xB1: {
            LDA(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0xB4: {
            LDY(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0xB5: {
            LDA(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0xB6: {
            LDX(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_Y))); cpu->cycles += 4; break; }
        case 0xB8: {
            CLV(cpu); cpu->cycles += 2; break; }
        case 0xB9: {
            LDA(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0xBA: {
            TSX(cpu); cpu->cycles += 2; break; }
        case 0xBC: {
            LDY(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0xBD: {
            LDA(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0xBE: {
            LDX(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0xC0: {
            CPY(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xC1: {
            CMP(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0xC4: {
            CPY(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xC5: {
            CMP(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xC6: {
            DEC(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0xC8: {
            INY(cpu); cpu->cycles += 2; break; }
        case 0xC9: {
            CMP(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xCA: {
            DEX(cpu); cpu->cycles += 2; break; }
        case 0xCC: {
            CPY(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xCD: {
            CMP(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xCE: {
            DEC(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0xD0: {
            BNE(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0xD1: {
            CMP(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0xD5: {
            CMP(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0xD6: {
            DEC(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0xD8: {
            CLD(cpu); cpu->cycles += 2; break; }
        case 0xD9: {
            CMP(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0xDD: {
            CMP(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0xDE: {
            DEC(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        case 0xE0: {
            CPX(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xE1: {
            SBC(cpu, readAddress(cpu->bus, AM_PREINDEXED_INDIRECT(cpu))); cpu->cycles += 6; break; }
        case 0xE4: {
            CPX(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xE5: {
            SBC(cpu, readAddress(cpu->bus, AM_ZPA(cpu))); cpu->cycles += 3; break; }
        case 0xE6: {
            INC(cpu, AM_ZPA(cpu)); cpu->cycles += 5; break; }
        case 0xE8: {
            INX(cpu); cpu->cycles += 2; break; }
        case 0xE9: {
            SBC(cpu, AM_IMM(cpu)); cpu->cycles += 2; break; }
        case 0xEA: {
            NOP(cpu); cpu->cycles += 2; break; }
        case 0xEC: {
            CPX(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xED: {
            SBC(cpu, readAddress(cpu->bus, AM_ABS(cpu))); cpu->cycles += 4; break; }
        case 0xEE: {
            INC(cpu, AM_ABS(cpu)); cpu->cycles += 6; break; }
        case 0xF0: {
            BEQ(cpu, AM_REL(cpu)); cpu->cycles += 2; break; }
        case 0xF1: {
            SBC(cpu, readAddress(cpu->bus, AM_POSTIDNEXED_INDIRECT(cpu, 1))); cpu->cycles += 5; break; }
        case 0xF5: {
            SBC(cpu, readAddress(cpu->bus, AM_INDEXED_ZPA(cpu, REG_X))); cpu->cycles += 4; break; }
        case 0xF6: {
            INC(cpu, AM_INDEXED_ZPA(cpu, REG_X)); cpu->cycles += 6; break; }
        case 0xF8: {
            SED(cpu); cpu->cycles += 2; break; }
        case 0xF9: {
            SBC(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_Y, 1))); cpu->cycles += 4; break; }
        case 0xFD: {
            SBC(cpu, readAddress(cpu->bus, AM_ABS_INDEXED(cpu, REG_X, 1))); cpu->cycles += 4; break; }
        case 0xFE: {
            INC(cpu, AM_ABS_INDEXED(cpu, REG_X, 0)); cpu->cycles += 7; break; }
        default: {
            fprintf(stderr, "Unknown OPCODE: %x at: %x", OPCODE, (cpu->PC - 1));
            return; }
    }
}

void SetNZFlags(CPU* cpu, byte resultByte){
    cpu->SR = (cpu->SR & ~FB_Z) | (resultByte == 0    ? FB_Z : 0);
    cpu->SR = (cpu->SR & ~FB_N) | (resultByte &  0x80 ? FB_N : 0); // Checking the MSB.
}

void LDA(CPU* cpu, byte data){
    cpu->A = data;
    SetNZFlags(cpu, cpu->A);
}

void LDX(CPU* cpu, byte data){
    cpu->X = data;
    SetNZFlags(cpu, cpu->X);
}

void LDY(CPU* cpu, byte data){
    cpu->Y = data;
    SetNZFlags(cpu, cpu->Y);
}

void STA(CPU* cpu, word address){
    cpu->bus->mem[address] = cpu->A;
}

void TAX(CPU* cpu){
    cpu->X = cpu->A;
    SetNZFlags(cpu, cpu->X);
}

void TAY(CPU* cpu){
    cpu->Y = cpu->A;
    SetNZFlags(cpu, cpu->Y);
}

void TXA(CPU* cpu){
    cpu->A = cpu->X;
    SetNZFlags(cpu, cpu->A);
}

void TYA(CPU* cpu){
    cpu->A = cpu->Y;
    SetNZFlags(cpu, cpu->A);
}

void JMP(CPU* cpu, word address){
    cpu->PC = address;
}

// Since the relative address can be negative, I use i8 instead of the byte, since both the uint16_t and i8_t will get promoted to int32_t and not uint32_t, there should be no errors.
void BEQ(CPU* cpu, i8 relAddr){
    byte hi = (byte)(cpu->PC >> 8);
    if((cpu->SR & FB_Z) != 0)
        cpu->PC = cpu->PC + relAddr;
    else
        return;
    byte newHi = (byte)(cpu->PC >> 8);
    cpu->cycles++;
    if(hi != newHi)
        cpu->cycles++;
}

void BNE(CPU* cpu, i8 relAddr){
    byte hi = (byte)(cpu->PC >> 8);
    if((cpu->SR & FB_Z) == 0)
        cpu->PC = cpu->PC + relAddr;
    else
        return;
    byte newHi = (byte)(cpu->PC >> 8);
    cpu->cycles++;
    if(hi != newHi)
        cpu->cycles++;
}

void CLC(CPU* cpu){
    cpu->SR &= ~FB_C;
}

void SEC(CPU* cpu){
    cpu->SR |=  FB_C;
}

void CLD(CPU* cpu){
    cpu->SR &= ~FB_D;
}

void SED(CPU* cpu){
    cpu->SR |=  FB_D;
}

void CLI(CPU* cpu){
    cpu->SR &= ~FB_IRQD;
}

void SEI(CPU* cpu){
    cpu->SR |=  FB_IRQD;
}

void CLV(CPU* cpu){
    cpu->SR &= ~FB_V;
}

void INX(CPU* cpu){
    cpu->X++;
    SetNZFlags(cpu, cpu->X);
}

void INY(CPU* cpu){
    cpu->Y++;
    SetNZFlags(cpu, cpu->Y);
}

void DEX(CPU* cpu){
    cpu->X--;
    SetNZFlags(cpu, cpu->X);
}

void DEY(CPU* cpu){
    cpu->Y--;
    SetNZFlags(cpu, cpu->Y);
}

void INC(CPU* cpu, word address){
    cpu->bus->mem[address]++;
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void DEC(CPU* cpu, word address){
    cpu->bus->mem[address]--;
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void AND(CPU* cpu, byte data){
    cpu->A &= data;
    SetNZFlags(cpu, cpu->A);
}

void ORA(CPU* cpu, byte data){
    cpu->A |= data;
    SetNZFlags(cpu, cpu->A);
}

void EOR(CPU* cpu, byte data){
    cpu->A ^= data;
    SetNZFlags(cpu, cpu->A);
}

void TXS(CPU* cpu){
    cpu->SP = cpu->X;
}

void TSX(CPU* cpu){
    cpu->X = cpu->SP;
    SetNZFlags(cpu, cpu->SP);
}

void STX(CPU* cpu, word address){
    cpu->bus->mem[address] = cpu->X;
}

void STY(CPU* cpu, word address){
    cpu->bus->mem[address] = cpu->Y;
}

void BCC(CPU* cpu, i8 relAddr){
    if(!(cpu->SR & FB_C)){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void BCS(CPU* cpu, i8 relAddr){
    if(cpu->SR & FB_C){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void BMI(CPU* cpu, i8 relAddr){
    if(cpu->SR & FB_N){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void BPL(CPU* cpu, i8 relAddr){
    if(!(cpu->SR & FB_N)){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void BVC(CPU* cpu, i8 relAddr){
    if(!(cpu->SR & FB_V)){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void BVS(CPU* cpu, i8 relAddr){
    if(cpu->SR & FB_V){
        word old = cpu->PC;
        cpu->PC += relAddr;
        cpu->cycles++;
        if((old & 0xFF00) != (cpu->PC & 0xFF00))
            cpu->cycles++;
    }
}

void CMP(CPU* cpu, byte data){
    byte temp = cpu->A - data;
    SetNZFlags(cpu, temp);
    if(cpu->A >= data)
        SEC(cpu);
    else
        CLC(cpu);
}

void CPX(CPU* cpu, byte data){
    byte temp = cpu->X - data;
    SetNZFlags(cpu, temp);
    if(cpu->X >= data)
        SEC(cpu);
    else
        CLC(cpu);
}

void CPY(CPU* cpu, byte data){
    byte temp = cpu->Y - data;
    SetNZFlags(cpu, temp);
    if(cpu->Y >= data)
        SEC(cpu);
    else
        CLC(cpu);
}

void ASL_ACC(CPU* cpu){
    if(cpu->A & 0x80)
        SEC(cpu);
    else
        CLC(cpu);
    cpu->A = (cpu->A << 1);
    SetNZFlags(cpu, cpu->A);
}

void ASL_MEM(CPU* cpu, word address){
    if(cpu->bus->mem[address] & 0x80)
        SEC(cpu);
    else
        CLC(cpu);
    cpu->bus->mem[address] = (cpu->bus->mem[address] << 1);
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void LSR_ACC(CPU* cpu){
    if(cpu->A & 0x01)
        SEC(cpu);
    else
        CLC(cpu);
    cpu->A = (cpu->A >> 1);
    SetNZFlags(cpu, cpu->A);
}

void LSR_MEM(CPU* cpu, word address){
    if(cpu->bus->mem[address] & 0x01)
        SEC(cpu);
    else
        CLC(cpu);
    cpu->bus->mem[address] = (cpu->bus->mem[address] >> 1);
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void ROL_ACC(CPU* cpu){
    byte prevByte = cpu->A;
    cpu->A = (cpu->A << 1);
    if((cpu->SR & FB_C) != 0)
        cpu->A |= 0x01;
    if(prevByte & 0x80)
        SEC(cpu);
    else
        CLC(cpu);
    SetNZFlags(cpu, cpu->A);
}

void ROL_MEM(CPU* cpu, word address){
    byte prevByte = cpu->bus->mem[address];
    cpu->bus->mem[address] = (cpu->bus->mem[address] << 1);
    if((cpu->SR & FB_C) != 0)
        cpu->bus->mem[address] |= 0x01;
    if(prevByte & 0x80)
        SEC(cpu);
    else
        CLC(cpu);
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void ROR_ACC(CPU* cpu){
    byte prevByte = cpu->A;
    cpu->A = (cpu->A >> 1);
    if((cpu->SR & FB_C) != 0)
        cpu->A |= 0x80;
    if(prevByte & 0x01)
        SEC(cpu);
    else
        CLC(cpu);
    SetNZFlags(cpu, cpu->A);
}

void ROR_MEM(CPU* cpu, word address){
    byte prevByte = cpu->bus->mem[address];
    cpu->bus->mem[address] = (cpu->bus->mem[address] >> 1);
    if((cpu->SR & FB_C) != 0)
        cpu->bus->mem[address] |= 0x80;
    if(prevByte & 0x01)
        SEC(cpu);
    else
        CLC(cpu);
    SetNZFlags(cpu, cpu->bus->mem[address]);
}

void PHA(CPU* cpu){
    cpu->bus->mem[0x0100 | (word)cpu->SP] = cpu->A;
    cpu->SP--;
}

void PLA(CPU* cpu){
    cpu->A = cpu->bus->mem[0x0100 | (word)++cpu->SP];
    SetNZFlags(cpu, cpu->A);
}

void PHP(CPU* cpu){
    cpu->bus->mem[0x0100 | (word)cpu->SP] = cpu->SR | 0x30;
    cpu->SP--;

}
void PLP(CPU* cpu){
    cpu->SR = (cpu->bus->mem[0x0100 | (word)++cpu->SP] | FB_IGNORED) & ~FB_B;
}

void JSR(CPU* cpu, word address){
    word ret = cpu->PC - 1; // 6502 quirk moment, add 1 when returning.
    cpu->PC = address;
    cpu->bus->mem[0x0100 | (word)cpu->SP--] = (ret >> 8);
    cpu->bus->mem[0x0100 | (word)cpu->SP--] = (byte)ret;
}

void RTS(CPU* cpu){
    byte retLo = cpu->bus->mem[0x0100 | (word)++cpu->SP];
    byte retHi = cpu->bus->mem[0x0100 | (word)++cpu->SP];
    cpu->PC = (((word)retHi << 8) | retLo) + 1;
}

void RTI(CPU* cpu){
    PLP(cpu);
    cpu->SR &= ~FB_B; 
    byte retLo = cpu->bus->mem[0x0100 | (word)++cpu->SP];
    byte retHi = cpu->bus->mem[0x0100 | (word)++cpu->SP];
    cpu->PC = (((word)retHi << 8) | retLo);
}

void ADC(CPU* cpu, byte data){
    word result = (word)cpu->A + (word)data;
    if((cpu->SR & FB_C) != 0)
        result++;
    if(~(cpu->A ^ data) & (cpu->A ^ result) & 0x80) // Overflow = when two same sign stuff give an output of different sign.
        cpu->SR |= FB_V;
    else
        cpu->SR &= ~FB_V;
    if(result > 0xFF)
        SEC(cpu);
    else
        CLC(cpu);
    cpu->A = (byte)result;
    SetNZFlags(cpu, cpu->A);
}

void SBC(CPU* cpu, byte data){
    ADC(cpu, ~data);
}

void BIT(CPU* cpu, byte data){
    byte value = cpu->A & data;
    SetNZFlags(cpu, value); // Set zero flag.
    byte newNV = data & 0b11000000;
    cpu->SR = (cpu->SR & 0b00111111) | newNV;
}

void BRK(CPU* cpu){
    word ret = cpu->PC + 1;
    cpu->bus->mem[0x0100 | (word)cpu->SP--] = (ret >> 8);
    cpu->bus->mem[0x0100 | (word)cpu->SP--] = (byte)ret;
    cpu->bus->mem[0x0100 | (word)cpu->SP--] = cpu->SR | FB_B;
    SEI(cpu);
    cpu->PC = (word)cpu->bus->mem[0xFFFE] | ((word)cpu->bus->mem[0xFFFF] << 8);
}

void NOP(CPU* cpu){
    return;
}
