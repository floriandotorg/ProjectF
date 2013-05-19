#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void dump_stack(cpu_t *cpu, uint8_t words)
{
    uint32_t sp = cpu->sp;
    
    puts("Stack Dump");
    
    for(;sp != 0x00FFFFFC && sp - cpu->sp < 40; sp += 4)
    {
        printf("%08x = 0x%08x\n", sp+4, cpu_read(sp+4,cpu));
    }
}

int main(int argc, char *argv[])
{
    cpu_t *cpu = cpu_create();
    uint8_t opcode;

    FILE *file = NULL;
    char *dump_ram = NULL;
    char *dump_flash = NULL;
    char **buffer = NULL;
    int i;

    if(argc != 2 && argc != 4 && argc != 6)
    {
        puts("usage: fsim <in> [--dumpram|-r <ram filename>] [--dumpflash|-f <flash filename>]");
        return EXIT_SUCCESS;
    }
    for (i = 2; i < argc; i++)
    {
        if (memcmp("--dumpram", argv[i], 9) == 0 || memcmp("-r", argv[i], 2) == 0)
        {
            buffer = &dump_ram;
        }
        else if (memcmp("--dumpflash", argv[i], 11) == 0 || memcmp("-f", argv[i], 2) == 0)
        {
            buffer = &dump_flash;
        }
        else if (buffer)
        {
            *buffer = argv[i];
            buffer = NULL;
        }
        else
        {
            printf("unknown option \"%s\"\n", argv[i]);
            return EXIT_FAILURE;
        }
    }
    if (buffer)
    {
         puts("Not all options set");
         return EXIT_FAILURE;
    }

    file = fopen(argv[1], "r");

    if(!file)
    {
        printf("could not open file \"%s\"",argv[1]);
        return EXIT_FAILURE;
    }

    fread(cpu->flash, sizeof(cpu->flash), 1, file);

    fclose(file);
    file = NULL;

    printf("First 160 bytes of flash:");
    for(i = 0;i < 160; i++) {
        if (i % 16 == 0)
        {
            printf("\n%08x:", i + 0x01000000);
        }
        if (i % 2 == 0)
        {
            printf(" ");
        }
        printf("%02x", cpu->flash[i]);
    }
    printf("\n\n");

    while(!cpu->status)
    {
        opcode = cpu_step(cpu);
    }
    switch (cpu->status)
    {
        case 2:
            printf("Illegal opcode \"%02x\"\n", opcode);
            break;
        case 1:
            puts("");
            puts("####################");
            puts("#        ##        #");
            puts("#### Halted CPU ####");
            puts("#        ##        #");
            puts("####################");
            puts("");
            break;
        default:
            printf("Unknown exit status %d", cpu->status);
    }

    puts("Register Dump:");
    printf("A  = %08x\n", cpu->a);
    printf("X  = %08x\n", cpu->x);
    printf("PC = %08x\n", cpu->pc);
    printf("SP = %08x\n", cpu->sp);
    printf("z  = %01d\n", cpu->z);
    printf("n  = %01d\n", cpu->n);
    printf("i  = %01d\n", cpu->i);
    printf("if = %02x\n", cpu->interrupt_flags);
    printf("iv = %08x\n", cpu->interrupt_vector);
    puts("");
    dump_stack(cpu, 10);

    if (dump_flash)
    {
        file = fopen(dump_flash, "w");
        if(!file)
        {
            printf("could not open flash file \"%s\"", dump_flash);
        } else {
            fwrite(cpu->flash, sizeof(cpu->flash), 1, file);
            fclose(file);
            printf("Dumped flash to \"%s\"\n", dump_flash);
        }
    }

    if (dump_ram)
    {
        file = fopen(dump_ram, "w");
        if(!file)
        {
            printf("could not open ram file \"%s\"", dump_ram);
        } else {
            fwrite(cpu->ram, sizeof(cpu->ram), 1, file);
            fclose(file);
            printf("Dumped ram to \"%s\"\n", dump_ram);
        }
    }
    cpu = cpu_free(cpu);

    return 0;
}
