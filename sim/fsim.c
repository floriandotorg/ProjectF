#include "uart.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

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

    uart_t *uart;
    uint32_t interrupt_vector;
    uint8_t interrupt_flags;
} cpu_t;

const uint32_t stack_pointer_start = 0x00FFFFFC;
const uint32_t byte_mask = 0x000000FF;

uint8_t read_byte_part(uint8_t part, uint32_t part_address, cpu_t *cpu)
{
    if ((part_address & 0xFF000000) != 0)
    {
        printf("Accessing outside the part %06x in %02x\n", part_address, part);
        return 0;
    }
    switch (part) {
    case 0x00: // RAM
        return cpu->ram[part_address];
    case 0x01: // Flash
        return cpu->flash[part_address];
    case 0xFF: // Periphery
        switch (part_address)
        {
        case 0x000003: // Status
            return uart_read_status(cpu->uart);
        case 0x000007: // Send
            return uart_read_recv(cpu->uart);
        case 0x0000E0: // Interrupt vector
        case 0x0000E1:
        case 0x0000E2:
        case 0x0000E3:
            return (cpu->interrupt_vector >> ((part_address - 0x0000E0) * 8)) & 0xFF;
        case 0x0000F1: // Interrupt flags
            return cpu->interrupt_flags;
        default:
            printf("Unknown periphery address %06x\n", part_address);
            return 0;
        }
    default:
        printf("Invalid adress %08x\n", ((uint32_t) part << 24) | part_address);
        return 0;
    }
}

uint8_t read_byte(uint32_t address, cpu_t *cpu)
{
    uint8_t part = (address & 0xFF000000) >> 24;
    uint32_t part_address = address & 0x00FFFFFF;

    return read_byte_part(part, part_address, cpu);
}

uint32_t read(uint32_t address, cpu_t *cpu)
{
    uint8_t part = (address & 0xFF000000) >> 24;
    uint32_t part_address = address & 0x00FFFFFF;

    uint32_t value = 0;
    uint8_t i = 0;
    for (; i < 4; i++)
    {
        value |= (uint32_t) read_byte_part(part, part_address + i, cpu) << (i * 8);
    }
    return value;
}

void write_byte_in_word(uint32_t *word, uint8_t byte, uint8_t index)
{
    *word = ((uint32_t) byte << (index * 8)) | (*word & ~(byte_mask << (index * 8)));
}

void write_byte_part(uint8_t part, uint32_t part_address, uint8_t value, cpu_t *cpu)
{
    if ((part_address & 0xFF000000) != 0)
    {
        printf("Accessing outside the part %06x in %02x\n", part_address, part);
        return;
    }
    else
    {
        switch (part) {
        case 0x00: // RAM
            cpu->ram[part_address] = value;
            break;
        case 0x01: // Flash
            cpu->flash[part_address] = value;
            break;
        case 0xFF: // Periphery
            switch (part_address)
            {
            case 0x000004: // Control
                uart_write_control(cpu->uart, value);
                break;
            case 0x000006: // Send
                uart_write_send(cpu->uart, value);
                break;
            case 0x0000E0: // Interrupt vector
            case 0x0000E1:
            case 0x0000E2:
            case 0x0000E3:
                write_byte_in_word(&cpu->interrupt_vector, value, part_address - 0x0000E0);
                break;
            case 0x0000F1: // Interrupt flags
                cpu->interrupt_flags = value;
                break;
            default:
                printf("Unknown periphery address %06x\n", part_address);
                break;
            }
            break;
        default:
            printf("Invalid adress %08x\n", ((uint32_t) part << 24) | part_address);
            break;
        }
    }
}

void write_byte(uint32_t address, uint8_t value, cpu_t *cpu)
{
    uint8_t part = (address & 0xFF000000) >> 24;
    uint32_t part_address = address & 0x00FFFFFF;

    write_byte_part(part, part_address, value, cpu);
}

void write(uint32_t address, uint32_t value, cpu_t *cpu)
{
    uint8_t part = (address & 0xFF000000) >> 24;
    uint32_t part_address = address & 0x00FFFFFF;

    uint8_t i = 0;
    for (; i < 4; i++)
    {
         write_byte_part(part, part_address + i, value & 0xFF, cpu);
         value >>= 8;
    }
}

uint32_t imm(cpu_t *cpu)
{
    uint32_t value = read(cpu->pc, cpu);
    cpu->pc += 4;
    return value;
}

uint32_t abs_address(cpu_t *cpu)
{
    return imm(cpu);
}

uint32_t ind_x_address(cpu_t *cpu)
{
    return read(imm(cpu) + cpu->x, cpu);
}

uint32_t ind_off_x_address(cpu_t *cpu)
{
    return read(imm(cpu), cpu) + cpu->x;
}

uint32_t absolute(cpu_t *cpu)
{
    return read(abs_address(cpu), cpu);
}

uint32_t ind_x(cpu_t *cpu) {
    return read(ind_x_address(cpu), cpu);
}

uint32_t ind_off_x(cpu_t *cpu)
{
    return read(ind_off_x_address(cpu), cpu);
}

void jump(uint8_t mode, cpu_t *cpu)
{
    switch (mode)
    {
        case 0:
            cpu->pc = abs_address(cpu);
            break;
        case 1:
            cpu->pc = ind_x_address(cpu);
            break;
        case 2:
            cpu->pc = ind_off_x_address(cpu);
            break;
        default :
            printf("Unknown jump mode %d\n", mode);
            break;
    }
}

void branch_zero(uint8_t zero, uint32_t target, cpu_t *cpu)
{
    if (cpu->z == zero)
    {
        cpu->pc = target;
    }
}

void branch_gl(uint8_t negative, uint32_t target, cpu_t *cpu)
{
    if (negative == cpu->n && cpu->z == 0)
    {
        cpu->pc = target;
    }
}

void dump_stack(cpu_t *cpu, uint8_t words)
{
    uint32_t sp = cpu->sp;
    
    puts("Stack Dump");
    
    for(;sp != stack_pointer_start && sp - cpu->sp < 40; sp += 4)
    {
        printf("%08x = 0x%08x\n", sp+4, read(sp+4,cpu));
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
    write(cpu->sp, value, cpu);
    cpu->sp -= 4;
}

uint32_t pop(cpu_t *cpu)
{
    cpu->sp += 4;
    return read(cpu->sp, cpu);
}

void pof(cpu_t *cpu)
{
    uint32_t pofed = pop(cpu);
    cpu->z = (pofed & 0b100) == 0 ? 0 : 1;
    cpu->n = (pofed & 0b010) == 0 ? 0 : 1;
    cpu->i = (pofed & 0b001) == 0 ? 0 : 1;
}

void jts(uint32_t target, cpu_t *cpu)
{
    push(cpu->pc, cpu);
    cpu->pc = target;
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
    printf("\n\n");

    cpu->a = 0;
    cpu->x = 0;
    cpu->pc = 0x01000000;
    cpu->sp = stack_pointer_start;
    cpu->z = 0;
    cpu->n = 0;
    cpu->i = 0; // By default off → no interrupt table defined
    cpu->interrupt_flags = 0;

    // UART
    cpu->uart = uart_create();

    while(!halted) {
      opcode = read_byte(cpu->pc++, cpu);
      //printf("%02x\n", opcode);
      switch (opcode) {
      // LDAB
        case 0x7F:
            load_byte(&cpu->a, read_byte(abs_address(cpu), cpu));
            set_zero_a(cpu);
            break;
        case 0x7E:
            load_byte(&cpu->a, read_byte(ind_x_address(cpu), cpu));
            set_zero_a(cpu);
            break;
        case 0x7D:
            load_byte(&cpu->a, read_byte(ind_off_x_address(cpu), cpu));
            set_zero_a(cpu);
            break;
        // LDXB
        case 0x70:
            load_byte(&cpu->x, read_byte(abs_address(cpu), cpu));
            set_zero_x(cpu);
            break;
        case 0x71:
            load_byte(&cpu->x, read_byte(ind_x_address(cpu), cpu));
            set_zero_x(cpu);
            break;
        case 0x72:
            load_byte(&cpu->x, read_byte(ind_off_x_address(cpu), cpu));
            set_zero_x(cpu);
            break;
        // STAB
        case 0x60:
            write_byte(abs_address(cpu), cpu->a, cpu);
            break;
        case 0x61:
            write_byte(ind_x_address(cpu), cpu->a, cpu);
            break;
        case 0x62:
            write_byte(ind_off_x_address(cpu), cpu->a, cpu);
            break;
        // STXB
        case 0x6F:
            write_byte(abs_address(cpu), cpu->x, cpu);
            break;
        case 0x6E:
            write_byte(ind_x_address(cpu), cpu->x, cpu);
            break;
        case 0x6D:
            write_byte(ind_off_x_address(cpu), cpu->x, cpu);
            break;
        // LDA
        case 0xAF:
            cpu->a = imm(cpu);
            set_zero_a(cpu);
            break;
        case 0xAE:
            cpu->a = absolute(cpu);
            set_zero_a(cpu);
            break;
        case 0xAD:
            cpu->a = ind_x(cpu);
            set_zero_a(cpu);
            break;
        case 0xAC:
            cpu->a = ind_off_x(cpu);
            set_zero_a(cpu);
            break;
        // LDX
        case 0xA0:
            cpu->x = imm(cpu);
            set_zero_x(cpu);
            break;
        case 0xA1:
            cpu->x = absolute(cpu);
            set_zero_x(cpu);
            break;
        case 0xA2:
            cpu->x = ind_x(cpu);
            set_zero_x(cpu);
            break;
        case 0xA3:
            cpu->x = ind_off_x(cpu);
            set_zero_x(cpu);
            break;
        // STA
        case 0x90:
            write(abs_address(cpu), cpu->a, cpu);
            break;
        case 0x91:
            write(ind_x_address(cpu), cpu->a, cpu);
            break;
        case 0x92:
            write(ind_off_x_address(cpu), cpu->a, cpu);
            break;
        // STX
        case 0x9D:
            write(abs_address(cpu), cpu->x, cpu);
            break;
        case 0x9E:
            write(ind_x_address(cpu), cpu->x, cpu);
            break;
        case 0x9F:
            write(ind_off_x_address(cpu), cpu->x, cpu);
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
        // PUA
        case 0xB2:
            push(cpu->a, cpu);
            set_zero_a(cpu);
            break;
        // PUX
        case 0xB3:
            push(cpu->x, cpu);
            set_zero_x(cpu);
            break;
        // PUF
        case 0xB6:
            push((cpu->z << 3) | (cpu->n << 2) | cpu-> i, cpu);
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
        // POF
        case 0xB7:
            pof(cpu);
            break;
      // AND
      case 0xF0:
        cpu->a = cpu->a & imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF1:
        cpu->a = cpu->a & absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xF2:
        cpu->a = cpu->a & ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xF3:
        cpu->a = cpu->a & ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // OR
      case 0xF4:
        cpu->a = cpu->a | imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF5:
        cpu->a = cpu->a | absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xF6:
        cpu->a = cpu->a | ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xF7:
        cpu->a = cpu->a | ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // XOR
      case 0xF8:
        cpu->a = cpu->a ^ imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xF9:
        cpu->a = cpu->a ^ absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xFA:
        cpu->a = cpu->a ^ ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xFB:
        cpu->a = cpu->a ^ ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // ROR
      case 0xFC:
        rot(imm(cpu), cpu);
        break;
      case 0xFD:
        rot(absolute(cpu), cpu);
        break;
      case 0xFE:
        rot(ind_x(cpu), cpu);
        break;
      case 0xFF:
        rot(ind_off_x(cpu), cpu);
        break;
      // ROL
      case 0xE1:
        rot(32 - imm(cpu), cpu);
        break;
      case 0xE2:
        rot(32 - absolute(cpu), cpu);
        break;
      case 0xE3:
        rot(32 - ind_x(cpu), cpu);
        break;
      case 0xE4:
        rot(32 - ind_off_x(cpu), cpu);
        break;
      // LSR
      case 0xE5:
        cpu->a = cpu->a >> imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xE6:
        cpu->a = cpu->a >> absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xE7:
        cpu->a = cpu->a >> ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xE8:
        cpu->a = cpu->a >> ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // LSL
      case 0xE9:
        cpu->a = cpu->a << imm(cpu);
        set_zero_a(cpu);
        break;
      case 0xEA:
        cpu->a = cpu->a << absolute(cpu);
        set_zero_a(cpu);
        break;
      case 0xEB:
        cpu->a = cpu->a << ind_x(cpu);
        set_zero_a(cpu);
        break;
      case 0xEC:
        cpu->a = cpu->a << ind_off_x(cpu);
        set_zero_a(cpu);
        break;
      // ADD
      case 0xC0:
        cpu->a = cpu->a + imm(cpu);
        set_flags_a(cpu);
        break;
      case 0xC1:
        cpu->a = cpu->a + absolute(cpu);
        set_flags_a(cpu);
        break;
      case 0xC2:
        cpu->a = cpu->a + ind_x(cpu);
        set_flags_a(cpu);
        break;
      case 0xC3:
        cpu->a = cpu->a + ind_off_x(cpu);
        set_flags_a(cpu);
        break;
      // CMP
      case 0xC4:
        cmp(imm(cpu), cpu);
        break;
      case 0xC5:
        cmp(absolute(cpu), cpu);
        break;
      case 0xC6:
        cmp(ind_x(cpu), cpu);
        break;
      case 0xC7:
        cmp(ind_off_x(cpu), cpu);
        break;
      // JMP
      case 0xD0:
        cpu->pc = abs_address(cpu);
        break;
      case 0xD1:
        cpu->pc = ind_x_address(cpu);
        break;
      case 0xD2:
        cpu->pc = ind_off_x_address(cpu);
        break;
      // BNE
      case 0xD3:
        branch_zero(0, abs_address(cpu), cpu);
        break;
      case 0xD4:
        branch_zero(0, ind_x_address(cpu), cpu);
        break;
      case 0xD5:
        branch_zero(0, ind_off_x_address(cpu), cpu);
        break;
      // BEQ
      case 0xDC:
        branch_zero(1, abs_address(cpu), cpu);
        break;
      case 0xDD:
        branch_zero(1, ind_x_address(cpu), cpu);
        break;
      case 0xDE:
        branch_zero(1, ind_off_x_address(cpu), cpu);
        break;
      // BGT
      case 0xD6:
        branch_gl(0, abs_address(cpu), cpu);
        break;
      case 0xD7:
        branch_gl(0, ind_x_address(cpu), cpu);
        break;
      case 0xD8:
        branch_gl(0, ind_off_x_address(cpu), cpu);
        break;
      // BLT
      case 0xD9:
        branch_gl(1, abs_address(cpu), cpu);
        break;
      case 0xDA:
        branch_gl(1, ind_x_address(cpu), cpu);
        break;
      case 0xDB:
        branch_gl(1, ind_off_x_address(cpu), cpu);
        break;
      // JTS
      case 0xBC:
        jts(abs_address(cpu), cpu);
        break;
      case 0xBD:
        jts(ind_x_address(cpu), cpu);
        break;
      case 0xBE:
        jts(ind_off_x_address(cpu), cpu);
        break;
      // RTS
      case 0xBF:
        cpu->pc = pop(cpu);
        break;
      // RTI
      case 0xB8:
        cpu->i = 1;
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
        printf("Illegal opcode \"%02x\"\n", opcode);
        halted = 1;
        break;
      // HLT
      case 0x83:
        puts("");
        puts("####################");
        puts("#        ##        #");
        puts("#### Halted CPU ####");
        puts("#        ##        #");
        puts("####################");
        puts("");
        halted = 1;
        break;
      }
      uart_recv_loop(cpu->uart, &cpu->interrupt_flags);
      if (cpu->interrupt_flags && cpu->i)
      {
          cpu->i = 0;
          jts(cpu->interrupt_vector, cpu);
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

    cpu->uart = uart_free(cpu->uart);

    free(cpu);
    cpu = NULL;

    return 0;
}
