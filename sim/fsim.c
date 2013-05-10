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
    uint32_t a;
    uint32_t x;
    uint32_t pc;
    uint32_t sp;
    uint8_t z;
    uint8_t n;
    uint8_t i;
    uint8_t ram[0x01000000];
    uint8_t flash[0x01000000];
} cpu_t;

const uint32_t stack_pointer_start = 0x00FFFFFC;
const uint32_t byte_mask = 0x000000FF;

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

void jump(uint8_t mode, cpu_t *cpu)
{
    switch (mode)
    {
        case 0:
            cpu->pc = *imm(cpu);
            break;
        case 1:
            cpu->pc = *val(*imm(cpu) + cpu->x, cpu);
            break;
        case 2:
            cpu->pc = *val(*imm(cpu), cpu) + cpu->x;
            break;
        default :
            printf("Unknown jump mode %d\n", mode);
            break;
    }
}

void branch_nz(uint8_t mode, cpu_t *cpu)
{
    if (cpu->z == 0)
    {
        jump(mode, cpu);
    }
}

void branch_gl(uint8_t negative, uint8_t mode, cpu_t *cpu)
{
    if (negative == cpu->n)
    {
        jump(mode, cpu);
    }
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

void set_flags_x(cpu_t *cpu)
{
    set_zero_x(cpu);
    cpu->n = cpu->x < 0 ? 1 : 0;
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

void cmp(uint32_t parameter, cpu_t *cpu)
{
    cpu->z = parameter == cpu->a ? 1 : 0;
    cpu->n = parameter  < cpu->a ? 1 : 0;
}

void rot(uint32_t delta, cpu_t *cpu)
{
    delta %= 32;
    uint32_t mask = ~(UINT32_C(1) << delta);
    cpu->a = ((cpu->a >> delta) & mask) | ((cpu->a & mask) << delta);
    set_zero_a(cpu);
}

void push(uint32_t value, cpu_t *cpu)
{
    *ram(cpu->sp, cpu) = value;
    cpu->sp -= 4;
}

uint32_t pop(cpu_t *cpu)
{
    cpu->sp += 4;
    return *ram(cpu->sp, cpu);
}

void jts(uint8_t mode, cpu_t *cpu)
{
    push(cpu->pc + 4, cpu);
    jump(mode, cpu);
}

void load_byte(uint32_t *reg, uint32_t value)
{
    *reg = (*reg & ~byte_mask) | (value & byte_mask);
}

int main(int argc, char *argv[])
{
    unsigned char opcode = 0x00;
    cpu_t *cpu = malloc(sizeof(*cpu));
    memset(cpu, 0, sizeof(*cpu));

    FILE *file = NULL;
    int halted = 0;
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
    printf("\n");

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
      // LDAB
      case 0x7F:
        load_byte(&cpu->a, *absolute(cpu));
        break;
      case 0x7E:
        load_byte(&cpu->a, *ind_x(cpu));
        break;
      case 0x7D:
        load_byte(&cpu->a, *ind_off_x(cpu));
        break;
      // LDXB
      case 0x70:
        load_byte(&cpu->x, *absolute(cpu));
        break;
      case 0x71:
        load_byte(&cpu->x, *ind_x(cpu));
        break;
      case 0x72:
        load_byte(&cpu->x, *ind_off_x(cpu));
        break;
      // STAB
      case 0x60:
        load_byte(absolute(cpu), cpu->a);
        break;
      case 0x61:
        load_byte(ind_x(cpu), cpu->a);
        break;
      case 0x62:
        load_byte(ind_off_x(cpu), cpu->a);
        break;
      // STXB
      case 0x6F:
        load_byte(absolute(cpu), cpu->x);
        break;
      case 0x6E:
        load_byte(ind_x(cpu), cpu->x);
        break;
      case 0x6D:
        load_byte(ind_off_x(cpu), cpu->x);
        break;
      // LDA
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
        push(cpu->a, cpu);
        set_zero_a(cpu);
        break;
      //PUX
      case 0xB3:
        push(cpu->x, cpu);
        set_zero_x(cpu);
        break;
      //POA
      case 0xB4:
        cpu->a = pop(cpu);
        set_zero_a(cpu);
        break;
      // POX
      case 0xB5:
        cpu->x = pop(cpu);
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
        rot(*imm(cpu), cpu);
        break;
      case 0xFD:
        rot(*absolute(cpu), cpu);
        break;
      case 0xFE:
        rot(*ind_x(cpu), cpu);
        break;
      case 0xFF:
        rot(*ind_off_x(cpu), cpu);
        break;
      // ROL
      case 0xE1:
        rot(32 - *imm(cpu), cpu);
        break;
      case 0xE2:
        rot(32 - *absolute(cpu), cpu);
        break;
      case 0xE3:
        rot(32 - *ind_x(cpu), cpu);
        break;
      case 0xE4:
        rot(32 - *ind_off_x(cpu), cpu);
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
        cmp(*imm(cpu), cpu);
        break;
      case 0xC5:
        cmp(*absolute(cpu), cpu);
        break;
      case 0xC6:
        cmp(*ind_x(cpu), cpu);
        break;
      case 0xC7:
        cmp(*ind_off_x(cpu), cpu);
        break;
      // JMP
      case 0xD0:
        jump(0, cpu);
        break;
      case 0xD1:
        jump(1, cpu);
        break;
      case 0xD2:
        jump(2, cpu);
        break;
      // BNE
      case 0xD3:
        branch_nz(0, cpu);
        break;
      case 0xD4:
        branch_nz(1, cpu);
        break;
      case 0xD5:
        branch_nz(2, cpu);
        break;
      // BGT
      case 0xD6:
        branch_gl(0, 0, cpu);
        break;
      case 0xD7:
        branch_gl(0, 1, cpu);
        break;
      case 0xD8:
        branch_gl(0, 2, cpu);
        break;
      // LGT
      case 0xD9:
        branch_gl(1, 0, cpu);
        break;
      case 0xDA:
        branch_gl(1, 1, cpu);
        break;
      case 0xDB:
        branch_gl(1, 2, cpu);
        break;
      // JTS
      case 0xDC:
        jts(0, cpu);
        break;
      case 0xDD:
        jts(1, cpu);
        break;
      case 0xDE:
        jts(2, cpu);
        break;
      // RTS
      case 0xDF:
        cpu->pc = pop(cpu);
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

    free(cpu);
    cpu = NULL;

    return 0;
}
