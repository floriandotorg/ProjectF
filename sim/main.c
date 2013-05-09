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

uint32_t *val(uint32_t a, cpu_t *cpu) {
    if (a < 0x01000000) {
        return (uint32_t *) &cpu->ram[a];
    } else if (a < 0x02000000) {
        return (uint32_t *) &cpu->flash[a - 0x01000000];
    } else {
        printf("Invalid adress %08x\n", a);
        return 0;
    }
}

uint32_t imm(cpu_t *cpu)
{
    uint32_t value = *val(cpu->pc, cpu);
    cpu->pc += 4;
    return value;
}

uint32_t absolute(cpu_t *cpu)
{
    return *val(imm(cpu), cpu);
}

uint32_t ind_x(cpu_t *cpu) {
    return *val(*val(imm(cpu) + cpu->x, cpu), cpu);
}

uint32_t ind_off_x(cpu_t *cpu)
{
    return *val(*val(imm(cpu), cpu) + cpu->x, cpu);
}

void dump_stack(cpu_t *cpu)
{
    int entries = 0x00FFFFFF - cpu->sp;
    printf("Dump: %d entries (max 10 shown)\n", entries);
    if (entries > 10)
    {
        entries = 10;
    }
    for (;entries > 0;entries--)
    {
        printf("%08x = 0x%02x\n", 0x00FFFFFF - entries, cpu->ram[0x00FFFFFF - entries]);
    }
}

int main(int argc, char *argv[])
{
    unsigned char opcode = 0x00;
    cpu_t *cpu = malloc(sizeof(*cpu));
    memset(cpu, 0, sizeof(*cpu));

    FILE *in = NULL;
    int halted = 0;

    if(argc != 2)
    {
        puts("usage: fsim <in>");
        return EXIT_SUCCESS;
    }

    in = fopen(argv[1], "r");

    if(!in)
    {
        printf("could not open file \"%s\"",argv[1]);
        return EXIT_FAILURE;
    }

    fread(cpu->flash, sizeof(cpu->flash), 1, in);

    int i = 0;
    for(;i < 12; i++) {
      printf("%02x", cpu->flash[i]);
      if (i % 2 == 1) {
        printf(" ");
      }
      if (i % 4 == 3) {
        printf("\n");
      }
    }

    fclose(in);
    in = NULL;

    cpu->a = 0;
    cpu->x = 0;
    cpu->pc = 0x01000000;
    cpu->sp = 0x00FFFFFF;
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
        cpu->a = imm(cpu);
        break;
      case 0xAE:
        cpu->a = absolute(cpu);
        break;
      case 0xAD:
        cpu->a = ind_x(cpu);
        break;
      case 0xAC:
        cpu->a = ind_off_x(cpu);
        break;
      case 0xA0:
        cpu->x = imm(cpu);
        break;
      case 0xA1:
        cpu->x = absolute(cpu);
        break;
      case 0xA2:
        cpu->x = ind_x(cpu);
        break;
      case 0xA3:
        cpu->x = ind_off_x(cpu);
        break;
      case 0x90:
      case 0x91:
      case 0x92:
        puts("STA\n");
        break;
      case 0x9D:
      case 0x9E:
      case 0x9F:
        puts("STX\n");
        break;
      // TXA
      case 0xA9:
        cpu->a = cpu->x;
        break;
      // TAX
      case 0xAA:
        cpu->x = cpu->a;
        break;
      // TXS
      case 0xB0:
        cpu->sp = cpu->x;
        break;
      // TSX
      case 0xB1:
        cpu->x = cpu->sp;
        break;
      //PUA
      case 0xB2:
        *val(cpu->sp--, cpu) = cpu->a;
        break;
      //PUX
      case 0xB3:
        *val(cpu->sp--, cpu) = cpu->x;
        break;
      //POA
      case 0xB4:
        cpu->a = *val(++cpu->sp, cpu);
        break;
      // POX
      case 0xB5:
        cpu->x = *val(++cpu->sp, cpu);
        break;
      case 0xF0:
        cpu->a = cpu->a & imm(cpu);
        break;
      case 0xF1:
        cpu->a = cpu->a & absolute(cpu);
        break;
      case 0xF2:
        cpu->a = cpu->a & ind_x(cpu);
        break;
      case 0xF3:
        cpu->a = cpu->a & ind_off_x(cpu);
        break;
      case 0xF4:
        cpu->a = cpu->a | imm(cpu);
        break;
      case 0xF5:
        cpu->a = cpu->a | absolute(cpu);
        break;
      case 0xF6:
        cpu->a = cpu->a | ind_x(cpu);
        break;
      case 0xF7:
        cpu->a = cpu->a | ind_off_x(cpu);
        break;
      // XOR
      case 0xF8:
        cpu->a = cpu->a ^ imm(cpu);
        break;
      case 0xF9:
        cpu->a = cpu->a ^ absolute(cpu);
        break;
      case 0xFA:
        cpu->a = cpu->a ^ ind_x(cpu);
        break;
      case 0xFB:
        cpu->a = cpu->a ^ ind_off_x(cpu);
        break;
      //ROR
      case 0xFC:
      case 0xFD:
      case 0xFE:
      case 0xFF:
        break;
      //ROL
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
        break;
      //LSR
      case 0xE5:
        cpu->a = cpu->a >> imm(cpu);
        break;
      case 0xE6:
        cpu->a = cpu->a >> absolute(cpu);
        break;
      case 0xE7:
        cpu->a = cpu->a >> ind_x(cpu);
        break;
      case 0xE8:
        cpu->a = cpu->a >> ind_off_x(cpu);
        break;
      //LSL
      case 0xE9:
        cpu->a = cpu->a << imm(cpu);
        break;
      case 0xEA:
        cpu->a = cpu->a << absolute(cpu);
        break;
      case 0xEB:
        cpu->a = cpu->a << ind_x(cpu);
        break;
      case 0xEC:
        cpu->a = cpu->a << ind_off_x(cpu);      
        cpu->z = cpu->a == 0;
        break;
      // ADD
      case 0xC0:
        cpu->a = cpu->a + imm(cpu);      
        cpu->z = cpu->a == 0;
        cpu->z = cpu->a < 0;
        break;
      case 0xC1:
        cpu->a = cpu->a + absolute(cpu);      
        cpu->z = cpu->a == 0;
        cpu->z = cpu->a < 0;
        break;
      case 0xC2:
        cpu->a = cpu->a + ind_x(cpu);      
        cpu->z = cpu->a == 0;
        cpu->z = cpu->a < 0;
        break;
      case 0xC3:
        cpu->a = cpu->a + ind_off_x(cpu);
        cpu->z = cpu->a == 0;
        cpu->z = cpu->a < 0;
        break;
      //CMP
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
/*      cpu->z = cpu->a == 0 ? 0 : 1;
        cpu->n = cpu->a < 0 ? 0 : 1;*/
        puts("CMP");
        break;
      //JMP
      case 0xD0:
        cpu->pc = imm(cpu);
        break;
      case 0xD1:
        //cpu->pc = ind_x(cpu);
      case 0xD2:
        puts("\"C\"-JMP");
        break;
      //BNE
      case 0xD3:
      case 0xD4:
      case 0xD5:
        puts("BNE");
        break;
      //BGT
      case 0xD6:
      case 0xD7:
      case 0xD8:
        puts("BGT");
        break;
      //LGT
      case 0xD9:
      case 0xDA:
      case 0xDB:
        puts("LGT");
        break;
      //JTS
      case 0xDC:
      case 0xDD:
      case 0xDE:
        puts("JTS");
        break;
      //RTS
      case 0xDF:
        puts("RTS");
        break;
      //INA
      case 0xC8:
        cpu->a++;
        break;
      //INX
      case 0xC9:
        cpu->x++;
        break;
      //DEA
      case 0xCA:
        cpu->a--;
        break;
      //DEX
      case 0xCB:
        cpu->x--;
        break;
      //SEI
      case 0x80:
        cpu->i = 1;
        break;
      //CLI
      case 0x81:
        cpu->i = 0;
        break;
      //NOP
      case 0x82:
        break;
      default:
        puts("Illegal opcode");
      //HLT
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
    dump_stack(cpu);

    free(cpu);
    cpu = NULL;

    return 0;
}
