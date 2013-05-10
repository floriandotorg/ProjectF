#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef union
{
    uint8_t bytes[4];
    uint32_t i;
} char_to_uint32_t;

typedef struct {
    int32_t a;
    int32_t x;
    uint32_t pc;
    uint32_t sp;
    int32_t z;
    int32_t n;
    int32_t i;
    uint8_t ram[0x01000000];
    uint8_t flash[0x01000000];
} cpu_t;

const uint32_t stack_pointer_start = 0x00FFFFFC;

uint32_t *ram(uint32_t a, cpu_t *cpu)
{
    return (uint32_t *) &cpu->ram[a];
}

uint32_t *val(uint32_t a, cpu_t *cpu)
{
    if (a < 0x01000000) {
        return ram(a, cpu);
    } else if (a < 0x02000000) {
        return (uint32_t *) &cpu->flash[a - 0x01000000];
    } else {
        printf("Invalid adress %08x\n", a);
        return 0;
    }
}

uint32_t *imm(cpu_t *cpu)
{
    uint32_t *value = val(cpu->pc, cpu);
    cpu->pc += 4;
    return value;
}

uint32_t *absolute(cpu_t *cpu)
{
    return val(*imm(cpu), cpu);
}

uint32_t *ind_x(cpu_t *cpu) {
    return val(*val(*imm(cpu) + cpu->x, cpu), cpu);
}

uint32_t *ind_off_x(cpu_t *cpu)
{
    return val(*val(*imm(cpu), cpu) + cpu->x, cpu);
}

void dump_stack(cpu_t *cpu, uint8_t words)
{
    int entries = stack_pointer_start - cpu->sp;
    printf("Dump: %d words", entries / 4);
    if (words > 0)
    {
        printf(" (max %d words shown)\n", words);
        if (entries > words * 4)
        {
            entries = words * 4;
        }
    }
    for (;entries > 0; entries -= 4)
    {
        printf("%08x = 0x%02x\n", stack_pointer_start - entries, *ram(stack_pointer_start - entries, cpu));
    }
}

void set_zero_x(cpu_t *cpu)
{
    cpu->z = cpu->x == 0 ? 1 : 0;
}

void set_zero_a(cpu_t *cpu)
{
    cpu->z = cpu->a == 0 ? 1 : 0;
}

void set_flags_a(cpu_t *cpu)
{
    set_zero_a(cpu);
    cpu->n = cpu->a < 0 ? 1 : 0;
}

int main(int argc, char *argv[])
{
    unsigned char opcode = 0x00;
    cpu_t *cpu = malloc(sizeof(*cpu));
    memset(cpu, 0, sizeof(*cpu));

    FILE *in = NULL;
    int halted = 0;
    int dump_ram = 0;
    int dump_flash = 0;
    int i;

    if(argc < 1 || argc > 4)
    {
        puts("usage: fsim <in> [--dumpram|-r] [--dumpflash|-f]");
        return EXIT_SUCCESS;
    }
    for (i = 2; i < argc; i++)
    {
        if (memcmp("--dumpram", argv[i], 9) == 0 || memcmp("-r", argv[2], 2) == 0)
        {
            dump_ram = 1;
        }
        else if (memcmp("--dumpflash", argv[3], 11) == 0 || memcmp("-f", argv[3], 2) == 0)
        {
            dump_flash = 1;
        }
        else
        {
            printf("unknown option \"%s\"\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    in = fopen(argv[1], "r");

    if(!in)
    {
        printf("could not open file \"%s\"",argv[1]);
        return EXIT_FAILURE;
    }

    fread(cpu->flash, sizeof(cpu->flash), 1, in);

    printf("First 120 bytes of flash:");
    for(i = 0;i < 120; i++) {
        if (i % 12 == 0)
        {
            printf("\n%08x:", i + 0x01000000);
        }
        if (i % 2 == 0)
        {
            printf(" ");
        }
        printf("%02x", cpu->flash[i]);
    }
    printf("\n");

    fclose(in);
    in = NULL;

    cpu->a = 0;
    cpu->x = 0;
    cpu->pc = 0x01000000;
    cpu->sp = stack_pointer_start;
    cpu->z = 0;
    cpu->n = 0;
    cpu->i = 1; // By default off → no interrupt table defined

/*    cpu->ram[0] = 0x13;
    cpu->ram[1] = 0x37;
    cpu->ram[2] = 0x74;
    cpu->ram[3] = 0x47;
    cpu->ram[0x0000A000] = 0x02;
    cpu->ram[0x0000A001] = 0x20;
    cpu->ram[0x0000A004] = 0x0A;
    cpu->ram[0x0000A005] = 0xA0;
    cpu->ram[0x00134200] = 0x00;
    cpu->ram[0x00134201] = 0xA0;
    cpu->ram[0x00134204] = 0xFF;
    cpu->ram[0x00134205] = 0xF0;
    cpu->ram[0x0000F0FF] = 0xBA;
    cpu->ram[0x0000F100] = 0XDC;
    cpu->flash[1] = 0x42;
    cpu->flash[2] = 0x13;
    puts("before\n");
    printf("%08x\n", *val(0, cpu));
    printf("%08x\n", *val(0x01000000 + 1, cpu));
    cpu->x = 4;
    printf("%08x\n", imm(cpu)); cpu->pc = 0x01000000;
    printf("%08x\n", absolute(cpu)); cpu->pc = 0x01000000;
    printf("%08x\n", ind_x(cpu)); cpu->pc = 0x01000000;
    printf("%08x\n", ind_off_x(cpu)); cpu->pc = 0x01000000;
    printf("%08x\n", cpu->pc);*/

    while(!halted) {
      opcode = (uint8_t) *val(cpu->pc++, cpu);
      switch (opcode) {
      case 0xAF:
        cpu->a = *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xAE:
        cpu->a = *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xAD:
        cpu->a = *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xAC:
        cpu->a = *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // LDX
      case 0xA0:
        cpu->x = *imm(cpu);
        set_zero_x(cpu);
        break;
      case 0xA1:
        cpu->x = *absolute(cpu);
        set_zero_x(cpu);
        break;
      case 0xA2:
        cpu->x = *ind_x(cpu);
        set_zero_x(cpu);
        break;
      case 0xA3:
        cpu->x = *ind_off_x(cpu);
        set_zero_x(cpu);
        break;
      // STA
      case 0x90:
        *absolute(cpu) = cpu->a;
        break;
      case 0x91:
        *ind_x(cpu) = cpu->a;
        break;
      case 0x92:
        *ind_off_x(cpu) = cpu->a;
        break;
      // STX
      case 0x9D:
        *absolute(cpu) = cpu->x;
        break;
      case 0x9E:
        *ind_x(cpu) = cpu->x;
        break;
      case 0x9F:
        *ind_off_x(cpu) = cpu->x;
        break;
      // TXA
      case 0xA9:
        cpu->a = cpu->x;
        set_zero_a(cpu);
        break;
      // TAX
      case 0xAA:
        cpu->x = cpu->a;
        set_zero_x(cpu);
        break;
      // TXS
      case 0xB0:
        cpu->sp = cpu->x;
        cpu->z = cpu->sp == 0 ? 1 : 0;
        break;
      // TSX
      case 0xB1:
        cpu->x = cpu->sp;
        set_zero_x(cpu);
        break;
      //PUA
      case 0xB2:
        *ram(cpu->sp, cpu) = cpu->a;
        cpu->sp -= 4;
        set_zero_a(cpu);
        break;
      //PUX
      case 0xB3:
        *ram(cpu->sp, cpu) = cpu->x;
        cpu->sp -= 4;
        set_zero_x(cpu);
        break;
      //POA
      case 0xB4:
        cpu->sp += 4;
        cpu->a = *ram(cpu->sp, cpu);
        set_zero_a(cpu);
        break;
      // POX
      case 0xB5:
        cpu->sp += 4;
        cpu->x = *ram(cpu->sp, cpu);
        set_zero_x(cpu);
        break;
      // AND
      case 0xF0:
        cpu->a = cpu->a & *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF1:
        cpu->a = cpu->a & *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xF2:
        cpu->a = cpu->a & *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xF3:
        cpu->a = cpu->a & *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // OR
      case 0xF4:
        cpu->a = cpu->a | *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF5:
        cpu->a = cpu->a | *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xF6:
        cpu->a = cpu->a | *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xF7:
        cpu->a = cpu->a | *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // XOR
      case 0xF8:
        cpu->a = cpu->a ^ *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF9:
        cpu->a = cpu->a ^ *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xFA:
        cpu->a = cpu->a ^ *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xFB:
        cpu->a = cpu->a ^ *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // ROR
      case 0xFC:
      case 0xFD:
      case 0xFE:
      case 0xFF:
        puts("ROR"); cpu->pc += 4;
        set_zero_a(cpu);
        break;
      // ROL
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
        puts("ROL"); cpu->pc += 4;
        set_zero_a(cpu);
        break;
      // LSR
      case 0xE5:
        cpu->a = cpu->a >> *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xE6:
        cpu->a = cpu->a >> *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xE7:
        cpu->a = cpu->a >> *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xE8:
        cpu->a = cpu->a >> *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // LSL
      case 0xE9:
        cpu->a = cpu->a << *imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xEA:
        cpu->a = cpu->a << *absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xEB:
        cpu->a = cpu->a << *ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xEC:
        cpu->a = cpu->a << *ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // ADD
      case 0xC0:
        cpu->a = cpu->a + *imm(cpu);
        set_flags_a(cpu);
        break;
      case 0xC1:
        cpu->a = cpu->a + *absolute(cpu);
        set_flags_a(cpu);
        break;
      case 0xC2:
        cpu->a = cpu->a + *ind_x(cpu);
        set_flags_a(cpu);
        break;
      case 0xC3:
        cpu->a = cpu->a + *ind_off_x(cpu);
        set_flags_a(cpu);
        break;
      // CMP
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
/*      cpu->z = cpu->a == 0 ? 0 : 1;
        cpu->n = cpu->a < 0 ? 0 : 1;*/
        puts("CMP"); cpu->pc += 4;
        break;
      // JMP
      case 0xD0:
        cpu->pc = *imm(cpu);
        break;
      case 0xD1:
        //cpu->pc = *ind_x(cpu);
      case 0xD2:
        puts("\"C\"-JMP"); cpu->pc += 4;
        break;
      // BNE
      case 0xD3:
      case 0xD4:
      case 0xD5:
        puts("BNE"); cpu->pc += 4;
        break;
      // BGT
      case 0xD6:
      case 0xD7:
      case 0xD8:
        puts("BGT"); cpu->pc += 4;
        break;
      // LGT
      case 0xD9:
      case 0xDA:
      case 0xDB:
        puts("LGT"); cpu->pc += 4;
        break;
      // JTS
      case 0xDC:
      case 0xDD:
      case 0xDE:
        puts("JTS"); cpu->pc += 4;
        break;
      // RTS
      case 0xDF:
        puts("RTS");
        break;
      // INA
      case 0xC8:
        cpu->a++;
        set_flags_a(cpu);
        break;
      // INX
      case 0xC9:
        cpu->x++;
        set_flags_x(cpu);
        break;
      // DEA
      case 0xCA:
        cpu->a--;
        set_flags_a(cpu);
        break;
      // DEX
      case 0xCB:
        cpu->x--;
        set_flags_x(cpu);
        break;
      // SEI
      case 0x80:
        cpu->i = 1;
        break;
      // CLI
      case 0x81:
        cpu->i = 0;
        break;
      // NOP
      case 0x82:
        break;
      default:
        puts("Illegal opcode");
      // HLT
      case 0x83:
        halted = 1;
        break;
      }
    }

    puts("Register Dump:");
    printf("A  = %08x\n", cpu->a);
    printf("X  = %08x\n", cpu->x);
    printf("PC = %08x\n", cpu->pc);
    printf("SP = %08x\n", cpu->sp);
    printf("z  = %01d\n", cpu->z);
    printf("n  = %01d\n", cpu->n);
    printf("i  = %01d\n", cpu->i);
    dump_stack(cpu, 10);

    if (dump_ram)
    {
//        fread(cpu->flash, sizeof(cpu->flash), 1, in);
    }

    free(cpu);
    cpu = NULL;

    return 0;
}
