#include "../include/cpu.h"
#include "string.h"

RAM CreateMemory(){
    RAM temp;
    memset(temp.data, 0, 0x10000);
    // Little endian.
    temp.data[0xFFFC] = 00;
    temp.data[0xFFFD] = 04;
    return temp;
}

byte readAddress(RAM* ram, word address){
    return ram->data[address];
}

CPU Reset(RAM* ram){
    CPU tempCPU = {0};
    tempCPU.SP = 0xFD;
    tempCPU.PC = (readAddress(ram, 0xFFFC) | (((word)readAddress(ram, 0xFFFD)) << 8));
    tempCPU.SR = 0b00100100;
    return tempCPU;
}
