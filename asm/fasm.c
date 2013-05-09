#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum
{
    lda_immediate,
    sta_absolute,
} instr_enum_t;

typedef struct instr
{
    instr_enum_t mnemonic; 
    uint32_t param;
    struct instr *next;
} instr_t;

void free_instr_tree(instr_t *tree)
{
    instr_t *del = NULL;
    
    while(tree)
    {
        del = tree;
        tree = tree->next;
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

int white_string(const char *str)
{
    for(;*str;++str)
    {
        if(!isspace(*str))
        {
            return 0;
        }
    }
    return 1;
}

instr_t* parse_instr(instr_t *tree, const char *line)
{
    instr_t instr;
    memset(&instr,0,sizeof(instr));
    
    if(white_string(line))
    {
        
    }
    else if(memcmp("sta",line,3) == 0)
    {
        instr.mnemonic = sta_absolute;
        instr.param = atoi(line + 5);
        tree = add_instr_tree(tree, instr);
    }
    else if(memcmp("lda",line,3) == 0)
    {
        instr.mnemonic = lda_immediate;
        instr.param = atoi(line + 5);
        tree = add_instr_tree(tree, instr);
    }
    
    return tree;
}

void print_instr_tree(instr_t *tree)
{
    for(;tree;tree = tree->next)
    {
#define CASE(x) case x: printf(#x); break;
        switch(tree->mnemonic)
        {
            CASE(lda_immediate)
            CASE(sta_absolute)
            default: puts("print_instr_tree: illegal mnemonic");
        }
#undef CASE
        printf(": %08x\n", tree->param);
    }
}

void generate_image(instr_t *tree, const char *filename)
{
    FILE *out = NULL;
    
    out = fopen(filename, "wb");
    
    if(!out)
    {
        printf("could not open file \"%s\"",filename);
        return;
    }

    for(;tree;tree = tree->next)
    {
        switch(tree->mnemonic)
        {
            case lda_immediate:
                fputc(0xaf, out);
                fwrite(&tree->param,sizeof(tree->param),1,out);
                break;
                
            case sta_absolute:
                fputc(0x90, out);
                fwrite(&tree->param,sizeof(tree->param),1,out);
                break; 
                
            default: puts("generate_image: illegal mnemonic");
        }
    }
    
    fclose(out);
    out = NULL;
}

void str_to_lower(char *str)
{
    for(;*str;++str)
    {
        *str = tolower(*str);
    }
}

int main(int argc, char *argv[])
{
    FILE *in = NULL;
    instr_t *tree = NULL;
    char line[16];
    
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
        str_to_lower(line);
        tree = parse_instr(tree, line);
    }
    
    fclose(in);
    in = NULL;
    
    print_instr_tree(tree);
    
    generate_image(tree, argv[2]);
    
    free_instr_tree(tree);
    
    return EXIT_SUCCESS;
}