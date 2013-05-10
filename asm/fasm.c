#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum
{
    invalid_instr,
    label,
    addr_offset,
    byte,
    word,
    string,
    ldab_absolute,
    ldab_indirect_x,
    ldab_indirect_off,
    ldxb_absolute,
    ldxb_indirect_x,
    ldxb_indirect_off,
    stab_absolute,
    stab_indirect_x,
    stab_indirect_off,
    stxb_absolute,
    stxb_indirect_x,
    stxb_indirect_off,
    lda_immediate,
    lda_absolute,
    lda_indirect_x,
    lda_indirect_off,
    ldx_immediate,
    ldx_absolute,
    ldx_indirect_x,
    ldx_indirect_off,
    sta_absolute,
    sta_indirect_x,
    sta_indirect_off,
    stx_absolute,
    stx_indirect_x,
    stx_indirect_off,
    txa,
    tax,
    txs,
    tsx,
    pua,
    pux,
    poa,
    pox,
    and_immediate,
    and_absolute,
    and_indirect_x,
    and_indirect_off,
    or_immediate,
    or_absolute,
    or_indirect_x,
    or_indirect_off,
    xor_immediate,
    xor_absolute,
    xor_indirect_x,
    xor_indirect_off,
    ror_immediate,
    ror_absolute,
    ror_indirect_x,
    ror_indirect_off,   
    rol_immediate,
    rol_absolute,
    rol_indirect_x,
    rol_indirect_off,     
    lsr_immediate,
    lsr_absolute,
    lsr_indirect_x,
    lsr_indirect_off, 
    lsl_immediate,
    lsl_absolute,
    lsl_indirect_x,
    lsl_indirect_off, 
    add_immediate,
    add_absolute,
    add_indirect_x,
    add_indirect_off, 
    cmp_immediate,
    cmp_absolute,
    cmp_indirect_x,
    cmp_indirect_off, 
    jmp_absolute,
    jmp_indirect_x,
    jmp_indirect_off, 
    bne_absolute,
    bne_indirect_x,
    bne_indirect_off, 
    bgt_absolute,
    bgt_indirect_x,
    bgt_indirect_off, 
    blt_absolute,
    blt_indirect_x,
    blt_indirect_off, 
    jts_absolute,
    jts_indirect_x,
    jts_indirect_off, 
    rts,
    ina,
    inx,
    dea,
    dex,
    sei,
    cli,
    nop,
    hlt,
} instr_enum_t;

typedef struct instr
{
    instr_enum_t mnemonic; 
    uint32_t param;
    char *str;
    struct instr *next;
} instr_t;

void free_instr_tree(instr_t *tree)
{
    instr_t *del = NULL;
    
    while(tree)
    {
        del = tree;
        tree = tree->next;
        
        if(del->str)
        {
            free(del->str);
        }
        free(del);
        del = NULL;
    }
}

instr_t* add_instr_tree(instr_t *tree, instr_t instr)
{
    instr_t *new_instr = NULL;
    
    if(!tree)
    {
        tree = malloc(sizeof(*tree));
        new_instr = tree;
    }
    else
    {
        new_instr = tree;
        for(;new_instr->next;new_instr = new_instr->next);
        new_instr->next = malloc(sizeof(*new_instr->next));
        new_instr = new_instr->next;
    }
    
    memcpy(new_instr, &instr, sizeof(*new_instr));
    
    return tree;
}

int label_string(const char *str)
{
    unsigned int count = 0;
    
    for(;*str;++str,++count)
    {
        if(!isalnum(*str) && *str != '_')
        {
            if(*str == ':')
            {
                return count;
            }
        }
    }
    return 0;
}

char* eat_whitespace(char *str)
{
    for(;*str && isspace(*str);++str);
    return str;
}

void parse_value(instr_t *instr, const char *val_str)
{
    char scanf_buf[50];
    size_t n;
    uint32_t result = 0;
    int sscanf_result = 0;
    
    memset(scanf_buf,0,sizeof(scanf_buf));
    
    printf("parse_value: %s", val_str);
    
    if(val_str[0] == '$')
    {
        sscanf_result = sscanf(val_str + 1, "%x", &result);
    }
    else if(val_str[0] == '%')
    {
        result = strtol(val_str + 1, NULL, 2);
        sscanf_result = 1;
    }
    else
    {
        sscanf_result = sscanf(val_str, "%i", &result);
        if(sscanf_result != 1)
        {
            for(n=0;isalnum(val_str[n])||val_str[n]=='_';++n)
            {
                scanf_buf[n]=val_str[n];
            }
            sscanf_result = 1;
        }
    }
    
    if(sscanf_result != 1)
    {
        printf("could not parse value: %s",val_str);
        exit(EXIT_FAILURE);
    }
    else
    {
        instr->param = result;
        if(scanf_buf[0])
        {
            instr->str = malloc(sizeof(char)*(strlen(scanf_buf)+1));
            strcpy(instr->str,scanf_buf);
        }
    }
}

int try_parse_instr(char *line, const char *instr_str, instr_t *instr, instr_enum_t immediate, instr_enum_t absolute, instr_enum_t indirect_off, instr_enum_t indirect_x)
{
    char *p = NULL;
    const size_t instr_str_len = strlen(instr_str);
    
    if(memcmp(instr_str,line,instr_str_len) == 0)
    {
        line += instr_str_len;
        line = eat_whitespace(line);
        
        if(line[0] == '#' && immediate != invalid_instr)
        {
            instr->mnemonic = immediate;
            parse_value(instr, line + 1);
        }
        else if(line[0] == '(')
        {
            p = strchr(line,',');
            if(!p)
            {
                printf("could not parse line: %s",line);
                exit(EXIT_FAILURE);
            }
            else if(*(p-1) == ')' && indirect_off != invalid_instr)
            {
                instr->mnemonic = indirect_off;
                parse_value(instr, line + 1);
            }
            else if(indirect_x != invalid_instr)
            {
                instr->mnemonic = indirect_x;
                parse_value(instr, line + 1);
            }
            else
            {
                printf("could not parse line: %s",line);
                exit(EXIT_FAILURE);
            }
        }
        else if(absolute != invalid_instr)
        {
            instr->mnemonic = absolute;
            parse_value(instr, line);
        }
        else
        {
            printf("could not parse line: %s",line);
            exit(EXIT_FAILURE);
        }
        
        return 1;
    }
    else
    {
        return 0;
    }
}

void str_to_lower(char *str)
{
    for(;*str;++str)
    {
        *str = tolower(*str);
    }
}

#define TRY_PARSE(x) else if(try_parse_instr(line, #x, &instr, x##_immediate, x##_absolute, x##_indirect_off, x##_indirect_x)) { }
#define TRY_PARSE_NO_IMMEDIATE(x) else if(try_parse_instr(line, #x, &instr, invalid_instr, x##_absolute, x##_indirect_off, x##_indirect_x)) { }
#define TRY_PARSE_NO_PARAMS(x) else if(memcmp(#x,line,strlen(#x)) == 0) { instr.mnemonic = x; }

instr_t* parse_instr(instr_t *tree, char *line)
{
    unsigned int pos;
    instr_t instr;

    memset(&instr,0,sizeof(instr));
    
    line = eat_whitespace(line);
    
    if(line[0] != '.')
    {
        str_to_lower(line);
    }
    
    if(!*line || line[0] == ';')
    {
        return tree;
    }
    else if((pos = label_string(line)))
    {
        instr.mnemonic = label;
        instr.str = malloc(sizeof(char) * (pos + 1));
        memset(instr.str, 0, sizeof(char) * (pos + 1));
        memcpy(instr.str, line, sizeof(char) * pos);
    }
    else if(line[0] == '.')
    {
        if(memcmp("byte",line+1,sizeof("byte")-1) == 0)
        {
            instr.mnemonic = byte;
            parse_value(&instr, line + 6);
        }
        else if(memcmp("word",line+1,sizeof("word")-1) == 0)
        {
            instr.mnemonic = word;
            parse_value(&instr, line + 6);
        }
        else if(memcmp("string",line+1,sizeof("string")-1) == 0)
        {
            instr.mnemonic = string;
            
            pos=strlen(line+8);
            instr.str = malloc(sizeof(char)*(pos+1));
            memset(instr.str,0,sizeof(char)*(pos+1));
            strcpy(instr.str,line+8);
            
            if(iscntrl(instr.str[pos-1]))
            {
                instr.str[pos-1] = '\0';
            }
        }
        else
        {
            printf("could not parse line: %s",line);
            exit(EXIT_FAILURE);
        }
    }
    else if(line[0] == '*')
    {
        line = eat_whitespace(line + 1);
        if(line[0] == '=')
        {
            line = eat_whitespace(line + 1);
            instr.mnemonic = addr_offset;
            parse_value(&instr, line);
        }
        else
        {
            printf("could not parse line: %s",line);
            exit(EXIT_FAILURE);
        }
    }
    TRY_PARSE_NO_IMMEDIATE(ldab)
    TRY_PARSE_NO_IMMEDIATE(ldxb)
    TRY_PARSE_NO_IMMEDIATE(stab)
    TRY_PARSE_NO_IMMEDIATE(stxb)
    TRY_PARSE(lda)
    TRY_PARSE(ldx)
    TRY_PARSE_NO_IMMEDIATE(sta)
    TRY_PARSE_NO_IMMEDIATE(stx)
    TRY_PARSE(and)
    TRY_PARSE(or)
    TRY_PARSE(xor)
    TRY_PARSE(ror)  
    TRY_PARSE(rol)
    TRY_PARSE(lsr)  
    TRY_PARSE(lsl)
    TRY_PARSE(add)  
    TRY_PARSE(cmp)  
    TRY_PARSE_NO_IMMEDIATE(jmp)
    TRY_PARSE_NO_IMMEDIATE(bne)
    TRY_PARSE_NO_IMMEDIATE(bgt)
    TRY_PARSE_NO_IMMEDIATE(blt)
    TRY_PARSE_NO_IMMEDIATE(jts)
    TRY_PARSE_NO_PARAMS(txa)
    TRY_PARSE_NO_PARAMS(tax)
    TRY_PARSE_NO_PARAMS(txs)
    TRY_PARSE_NO_PARAMS(tsx)
    TRY_PARSE_NO_PARAMS(pua)
    TRY_PARSE_NO_PARAMS(pux)
    TRY_PARSE_NO_PARAMS(poa)
    TRY_PARSE_NO_PARAMS(pox)        
    TRY_PARSE_NO_PARAMS(rts)
    TRY_PARSE_NO_PARAMS(ina)
    TRY_PARSE_NO_PARAMS(inx)
    TRY_PARSE_NO_PARAMS(dea)
    TRY_PARSE_NO_PARAMS(dex)
    TRY_PARSE_NO_PARAMS(sei)
    TRY_PARSE_NO_PARAMS(cli)
    TRY_PARSE_NO_PARAMS(nop)
    TRY_PARSE_NO_PARAMS(hlt)
    else
    {
        printf("could not parse line: %s",line);
        exit(EXIT_FAILURE);
    }
    
    tree = add_instr_tree(tree, instr);
    
    return tree;
}

#undef TRY_PARSE
#undef TRY_PARSE_NO_IMMEDIATE
#undef TRY_PARSE_NO_PARAMS

uint32_t instr_size(instr_t instr)
{
#define CASE(x) case x:
    switch(instr.mnemonic)
    {
        CASE(ldab_absolute)
        CASE(ldab_indirect_x)
        CASE(ldab_indirect_off)
        CASE(ldxb_absolute)
        CASE(ldxb_indirect_x)
        CASE(ldxb_indirect_off)
        CASE(stab_absolute)
        CASE(stab_indirect_x)
        CASE(stab_indirect_off)
        CASE(stxb_absolute)
        CASE(stxb_indirect_x)
        CASE(stxb_indirect_off)
        CASE(lda_immediate)
        CASE(lda_absolute)
        CASE(lda_indirect_x)
        CASE(lda_indirect_off)
        CASE(ldx_immediate)
        CASE(ldx_absolute)
        CASE(ldx_indirect_x)
        CASE(ldx_indirect_off)
        CASE(sta_absolute)
        CASE(sta_indirect_x)
        CASE(sta_indirect_off)
        CASE(stx_absolute)
        CASE(stx_indirect_x)
        CASE(stx_indirect_off)
        CASE(and_immediate)
        CASE(and_absolute)
        CASE(and_indirect_x)
        CASE(and_indirect_off)
        CASE(or_immediate)
        CASE(or_absolute)
        CASE(or_indirect_x)
        CASE(or_indirect_off)
        CASE(xor_immediate)
        CASE(xor_absolute)
        CASE(xor_indirect_x)
        CASE(xor_indirect_off)
        CASE(ror_immediate)
        CASE(ror_absolute)
        CASE(ror_indirect_x)
        CASE(ror_indirect_off)   
        CASE(rol_immediate)
        CASE(rol_absolute)
        CASE(rol_indirect_x)
        CASE(rol_indirect_off)     
        CASE(lsr_immediate)
        CASE(lsr_absolute)
        CASE(lsr_indirect_x)
        CASE(lsr_indirect_off) 
        CASE(lsl_immediate)
        CASE(lsl_absolute)
        CASE(lsl_indirect_x)
        CASE(lsl_indirect_off) 
        CASE(add_immediate)
        CASE(add_absolute)
        CASE(add_indirect_x)
        CASE(add_indirect_off) 
        CASE(cmp_immediate)
        CASE(cmp_absolute)
        CASE(cmp_indirect_x)
        CASE(cmp_indirect_off) 
        CASE(jmp_absolute)
        CASE(jmp_indirect_x)
        CASE(jmp_indirect_off) 
        CASE(bne_absolute)
        CASE(bne_indirect_x)
        CASE(bne_indirect_off) 
        CASE(bgt_absolute)
        CASE(bgt_indirect_x)
        CASE(bgt_indirect_off) 
        CASE(blt_absolute)
        CASE(blt_indirect_x)
        CASE(blt_indirect_off) 
        CASE(jts_absolute)
        CASE(jts_indirect_x)
        CASE(jts_indirect_off)
            return 5;
        
        CASE(txa)
        CASE(tax)
        CASE(txs)
        CASE(tsx)
        CASE(pua)
        CASE(pux)
        CASE(poa)
        CASE(pox)        
        CASE(rts)
        CASE(ina)
        CASE(inx)
        CASE(dea)
        CASE(dex)
        CASE(sei)
        CASE(cli)
        CASE(nop)
        CASE(hlt)
        CASE(byte)
            return 1;
            
        case word:
            return 4;
        
        case label:
            return 0;
            
        case string:
            return strlen(instr.str) + 1;
        
        default:
            puts("instr_size: illegal mnemonic");
            exit(EXIT_FAILURE);
    }
#undef CASE
}

void print_instr_tree(instr_t *tree)
{
    uint32_t cur_addr = 0;

    printf("%-12s%-20s%-12s%s\n","address","mnemonic","param","string");
    printf("%-12s%-20s%-12s%s\n","-------","--------","-----","------");
    
    for(;tree;tree = tree->next)
    {
        printf("%08x    ",cur_addr);
        
        if(tree->mnemonic == addr_offset)
        {
            cur_addr = tree->param;
        }
        else
        {
            cur_addr += instr_size(*tree);
        }
        
#define CASE(x) case x: printf("%-20s",#x); break;
        switch(tree->mnemonic)
        {
            CASE(invalid_instr)
            CASE(label)
            CASE(addr_offset)
            CASE(byte)
            CASE(word)
            CASE(string)
            CASE(ldab_absolute)
            CASE(ldab_indirect_x)
            CASE(ldab_indirect_off)
            CASE(ldxb_absolute)
            CASE(ldxb_indirect_x)
            CASE(ldxb_indirect_off)
            CASE(stab_absolute)
            CASE(stab_indirect_x)
            CASE(stab_indirect_off)
            CASE(stxb_absolute)
            CASE(stxb_indirect_x)
            CASE(stxb_indirect_off)
            CASE(lda_immediate)
            CASE(lda_absolute)
            CASE(lda_indirect_x)
            CASE(lda_indirect_off)
            CASE(ldx_immediate)
            CASE(ldx_absolute)
            CASE(ldx_indirect_x)
            CASE(ldx_indirect_off)
            CASE(sta_absolute)
            CASE(sta_indirect_x)
            CASE(sta_indirect_off)
            CASE(stx_absolute)
            CASE(stx_indirect_x)
            CASE(stx_indirect_off)
            CASE(txa)
            CASE(tax)
            CASE(txs)
            CASE(tsx)
            CASE(pua)
            CASE(pux)
            CASE(poa)
            CASE(pox)
            CASE(and_immediate)
            CASE(and_absolute)
            CASE(and_indirect_x)
            CASE(and_indirect_off)
            CASE(or_immediate)
            CASE(or_absolute)
            CASE(or_indirect_x)
            CASE(or_indirect_off)
            CASE(xor_immediate)
            CASE(xor_absolute)
            CASE(xor_indirect_x)
            CASE(xor_indirect_off)
            CASE(ror_immediate)
            CASE(ror_absolute)
            CASE(ror_indirect_x)
            CASE(ror_indirect_off)   
            CASE(rol_immediate)
            CASE(rol_absolute)
            CASE(rol_indirect_x)
            CASE(rol_indirect_off)     
            CASE(lsr_immediate)
            CASE(lsr_absolute)
            CASE(lsr_indirect_x)
            CASE(lsr_indirect_off) 
            CASE(lsl_immediate)
            CASE(lsl_absolute)
            CASE(lsl_indirect_x)
            CASE(lsl_indirect_off) 
            CASE(add_immediate)
            CASE(add_absolute)
            CASE(add_indirect_x)
            CASE(add_indirect_off) 
            CASE(cmp_immediate)
            CASE(cmp_absolute)
            CASE(cmp_indirect_x)
            CASE(cmp_indirect_off) 
            CASE(jmp_absolute)
            CASE(jmp_indirect_x)
            CASE(jmp_indirect_off) 
            CASE(bne_absolute)
            CASE(bne_indirect_x)
            CASE(bne_indirect_off) 
            CASE(bgt_absolute)
            CASE(bgt_indirect_x)
            CASE(bgt_indirect_off) 
            CASE(blt_absolute)
            CASE(blt_indirect_x)
            CASE(blt_indirect_off) 
            CASE(jts_absolute)
            CASE(jts_indirect_x)
            CASE(jts_indirect_off) 
            CASE(rts)
            CASE(ina)
            CASE(inx)
            CASE(dea)
            CASE(dex)
            CASE(sei)
            CASE(cli)
            CASE(nop)
            CASE(hlt)
            default: puts("print_instr_tree: illegal mnemonic");
        }
#undef CASE
        if(tree->str)
        {
            printf("%08x    %s\n", tree->param, tree->str);
        }
        else
        {
            printf("%08x\n", tree->param);
        }
    }
}

void eval_labels(instr_t *tree)
{
    instr_t *n, *m;
    uint32_t cur_addr = 0;
    
    for(n = tree;n;n = n->next)
    {
        if(n->mnemonic == addr_offset)
        {
            cur_addr = n->param;
        }
        else
        {
            cur_addr += instr_size(*n);
        }
        
        if(n->mnemonic == label)
        {
            n->param = cur_addr;
        }
    }
    
    for(n = tree;n;n = n->next)
    {
        if(n->mnemonic != label && n->mnemonic != string && n->str)
        {
            for(m = tree;m;m = m->next)
            {
                if(m->mnemonic == label && strcmp(n->str,m->str) == 0)
                {
                    n->param = m->param;
                    m = tree;
                    break;
                }
            }
            if(m != tree)
            {
                printf("could not evaluate label: %s",n->str);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void generate_image(instr_t *tree, const char *filename)
{
    FILE *out = NULL;
    
    out = fopen(filename, "wb");
    
    if(!out)
    {
        printf("could not create file \"%s\"",filename);
        exit(EXIT_FAILURE);
    }
    
    for(;tree;tree = tree->next)
    {
#define CASE(x,h) case x: fputc(h, out); break;
#define CASE_P(x,h) case x: fputc(h, out); fwrite(&tree->param,sizeof(tree->param),1,out); break;
        switch(tree->mnemonic)
        {
            CASE_P(ldab_absolute,0x7f)
            CASE_P(ldab_indirect_x,0x7e)
            CASE_P(ldab_indirect_off,0x7d)
            CASE_P(ldxb_absolute,0x70)
            CASE_P(ldxb_indirect_x,0x71)
            CASE_P(ldxb_indirect_off,0x72)
            CASE_P(stab_absolute,0x60)
            CASE_P(stab_indirect_x,0x61)
            CASE_P(stab_indirect_off,0x62)
            CASE_P(stxb_absolute,0x6D)
            CASE_P(stxb_indirect_x,0x6E)
            CASE_P(stxb_indirect_off,0x6F)
            CASE_P(lda_immediate,0xaf)
            CASE_P(lda_absolute,0xae)
            CASE_P(lda_indirect_x,0xad)
            CASE_P(lda_indirect_off,0xac)
            CASE_P(ldx_immediate,0xa0)
            CASE_P(ldx_absolute,0xa1)
            CASE_P(ldx_indirect_x,0xa2)
            CASE_P(ldx_indirect_off,0xa3)
            CASE_P(sta_absolute,0x90)
            CASE_P(sta_indirect_x,0x91)
            CASE_P(sta_indirect_off,0x92)
            CASE_P(stx_absolute,0x9d)
            CASE_P(stx_indirect_x,0x9e)
            CASE_P(stx_indirect_off,0x9f)
            CASE_P(and_immediate,0xf0)
            CASE_P(and_absolute,0xf1)
            CASE_P(and_indirect_x,0xf2)
            CASE_P(and_indirect_off,0xf3)
            CASE_P(or_immediate,0xf4)
            CASE_P(or_absolute,0xf5)
            CASE_P(or_indirect_x,0xf6)
            CASE_P(or_indirect_off,0xf7)
            CASE_P(xor_immediate,0xf8)
            CASE_P(xor_absolute,0xf9)
            CASE_P(xor_indirect_x,0xfa)
            CASE_P(xor_indirect_off,0xfb)
            CASE_P(ror_immediate,0xfc)
            CASE_P(ror_absolute,0xfd)
            CASE_P(ror_indirect_x,0xfe)
            CASE_P(ror_indirect_off,0xff)   
            CASE_P(rol_immediate,0xe1)
            CASE_P(rol_absolute,0xe2)
            CASE_P(rol_indirect_x,0xe3)
            CASE_P(rol_indirect_off,0xe4)     
            CASE_P(lsr_immediate,0xe5)
            CASE_P(lsr_absolute,0xe6)
            CASE_P(lsr_indirect_x,0xe7)
            CASE_P(lsr_indirect_off,0xe8) 
            CASE_P(lsl_immediate,0xe9)
            CASE_P(lsl_absolute,0xea)
            CASE_P(lsl_indirect_x,0xeb)
            CASE_P(lsl_indirect_off,0xec) 
            CASE_P(add_immediate,0xc0)
            CASE_P(add_absolute,0xc1)
            CASE_P(add_indirect_x,0xc2)
            CASE_P(add_indirect_off,0xc3) 
            CASE_P(cmp_immediate,0xc4)
            CASE_P(cmp_absolute,0xc5)
            CASE_P(cmp_indirect_x,0xc6)
            CASE_P(cmp_indirect_off,0xc7) 
            CASE_P(jmp_absolute,0xd0)
            CASE_P(jmp_indirect_x,0xd1)
            CASE_P(jmp_indirect_off,0xd2) 
            CASE_P(bne_absolute,0xd3)
            CASE_P(bne_indirect_x,0xd4)
            CASE_P(bne_indirect_off,0xd5) 
            CASE_P(bgt_absolute,0xd6)
            CASE_P(bgt_indirect_x,0xd7)
            CASE_P(bgt_indirect_off,0xd8) 
            CASE_P(blt_absolute,0xd9)
            CASE_P(blt_indirect_x,0xda)
            CASE_P(blt_indirect_off,0xdb) 
            CASE_P(jts_absolute,0xdc)
            CASE_P(jts_indirect_x,0xdd)
            CASE_P(jts_indirect_off,0xde) 
        
            CASE(txa,0xa9)
            CASE(tax,0xaa)
            CASE(txs,0xb0)
            CASE(tsx,0xb1)
            CASE(pua,0xb2)
            CASE(pux,0xb3)
            CASE(poa,0xb4)
            CASE(pox,0xb5)        
            CASE(rts,0xdf)
            CASE(ina,0xc8)
            CASE(inx,0xc9)
            CASE(dea,0xca)
            CASE(dex,0xcb)
            CASE(sei,0x80)
            CASE(cli,0x81)
            CASE(nop,0x82)
            CASE(hlt,0x83) 
            
            case byte:
                fputc((uint8_t)tree->param, out);
                break;
            
            case word:
                fwrite(&tree->param,sizeof(tree->param),1,out);
                break;
            
            case string:
                fwrite(tree->str,strlen(tree->str)+1,1,out);
                break;
            
            case addr_offset:
            case label:
                break;
            
            default:
                puts("generate_image: illegal mnemonic");
                exit(EXIT_FAILURE);
        }
#undef CASE
#undef CASE_P
    }
    
    fclose(out);
    out = NULL;
}

int main(int argc, char *argv[])
{
    FILE *in = NULL;
    instr_t *tree = NULL;
    char line[200];
    
    if(argc != 3)
    {
        puts("usage: fasm <in> <out>");
        return EXIT_SUCCESS;
    }
    
    in = fopen(argv[1], "r");
    
    if(!in)
    {
        printf("could not open file \"%s\"",argv[1]);
        return EXIT_FAILURE;
    }
    
    while(!feof(in))
    {
        memset(line,0,sizeof(line));
        fgets(line, sizeof(line), in); 
        tree = parse_instr(tree, line);
    }
    
    fclose(in);
    in = NULL;
    
    eval_labels(tree);
    
    print_instr_tree(tree);
    
    generate_image(tree, argv[2]);
    
    free_instr_tree(tree);
    
    return EXIT_SUCCESS;
}