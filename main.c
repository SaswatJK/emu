#include "./include/cpu.h"
#include <stdio.h>
#include <inttypes.h>

void PrintMemory(BUS* bus){
    for(int i = 0; i < 0x10000; i++){
        if(i % 16 == 0)
            printf("\n%04X: ", i);
        printf("%02X ", bus->mem[i]);
    }
    printf("\n");
}

int main(){
    BUS RAMBus = CreateMemory();
    FILE *f = fopen("../test/6502_functional_test.bin", "rb");
    if(f == NULL){
        perror("Can't open file");
        return 0;
    }
    fread(RAMBus.mem, 1, 0x10000, f);
    fclose(f);
    RAMBus.mem[0xFFFC] = 0x00;
    RAMBus.mem[0xFFFD] = 0x04;
    RAMBus.mem[0x0000] = 0x01; // 6502 doesn't have BCD mode afaik.
    CPU cpu = Reset(&RAMBus);
    while(1){
        word oldPC = cpu.PC;
        Step(&cpu);
        if(cpu.PC == oldPC){
            fprintf(stderr, "TRAPPED at 0x%04X. Cycles: %" PRIu64"\n", cpu.PC, cpu.cycles);
            //PrintMemory(cpu.bus);
            return 1;
        }
        if(cpu.PC == 0x3469){
            fprintf(stderr, "ALL TESTS PASSED. Cycles: %" PRIu64"\n", cpu.cycles);
            return 0;
        }
    }
}
