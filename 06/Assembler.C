/*This is the Assembler for project 6 of the nand2tetris project. It does what it's supposed to do, the only things I did not manage to implement
was whitespace removal, so the user can use indentation in the .asm files, and dynamic line sizes (the only thing
that is dynamic is the symbol table). Except that, the program works with all the provided test files by nand2tetris.*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_SIZE_ASM 655360
#define MAX_SIZE_ASM_LINE 1024
#define MAX_SIZE_HACK 65536

char asmb[MAX_SIZE_ASM][MAX_SIZE_ASM_LINE];
char hack[MAX_SIZE_HACK][MAX_SIZE_HACK];

typedef enum{
    A_COMMAND,
    C_COMMAND,
    L_COMMAND
}command_type;

typedef struct{
    char symbol[1024];
    int address;
}symbol_entry;
int hash_size=0;
int address_count=0;
int symbol_index=0;

void show_bits(unsigned a)
{
    for(int i=sizeof(a)*8-1; i>=0; i--)
        printf("%d", (a>>i)&1);
    printf("\n");
}

void delete_newline(char *line)
{
    //deleting carriage returns
    char auxline[MAX_SIZE_ASM_LINE];
    int k=0;
    for(int i=0; i<(int)strlen(line); i++)
    {
        if(line[i]!='\r')
            auxline[k++]=line[i];
    }
    auxline[k]='\0';
    strcpy(line, auxline);
    //deleting newlines
    for(int i=0; i<(int)strlen(line); i++)
        if(line[i]=='\n')
            {
                line[i]='\0';
                break;
            }
            
}

bool is_empty_line(const char *line) 
{
    for (int i=0; i<(int)strlen(line); i++)
    {
        if (line[i]!=' ' && line[i]!='\t' && line[i]!='\n' && line[i]!='\r')
            return false;
    }
    return true;
}

bool is_symbol(const char *sb)
{
    for(int i=0; i<(int)strlen(sb); i++)
        if(isdigit(sb[i])==0)
            return true;
    return false;
}

int open_asm(const char* filename)
{
    FILE *f=NULL;
    if((f=fopen(filename, "r"))==NULL)
    {
        fprintf(stderr, "error opening .asm file\n");
        exit(EXIT_FAILURE);
    }
    
    char line[MAX_SIZE_ASM_LINE];
    int size=0;
    while(fgets(line, MAX_SIZE_ASM_LINE, f)!=NULL)
    {
        if((line[0]=='/' && line[1]=='/') || is_empty_line(line))
            continue;
        //printf("%s\n", line);
        strcpy(asmb[size], line);
        size++;
    }
    printf(".asm file read successfully\n");

    if(fclose(f)!=0)
    {
        fprintf(stderr, "error closing .asm file\n");
        exit(EXIT_FAILURE);
    }
    return size;
}

void symbol(char *s, char *line, command_type cmd)
{
    s[0]='\0';
    if(cmd==A_COMMAND)
    {
        char *p=strtok(line, "@");
        strcpy(s, p);
    }
    else
    {
        int k=0;
        for(int i=0; i<(int)strlen(line); i++)
        {
            if(line[i]!='(' && line[i]!=')')
            {
                s[k]=line[i];
                k++;
            }
        }
    }
}

void dest(char *m, char *line)
{
    m[0]='\0';
    if(strchr(line, '=')==NULL)
        strcpy(m, "null");
    else if(line[0]=='M' && line[1]=='=')
        strcpy(m, "M");
    else if(line[0]=='D' && line[1]=='=')
        strcpy(m, "D");
    else if(line[0]=='M' && line[1]=='D')
        strcpy(m, "MD");
    else if(line[0]=='A' && line[1]=='=')
        strcpy(m, "A");
    else if(line[0]=='A' && line[1]=='M')
        strcpy(m, "AM");
    else if(line[0]=='A' && line[1]=='D')
        strcpy(m, "AD");
    else if(line[0]=='A' && line[1]=='M' && line[2]=='D')
        strcpy(m, "AMD");
    //printf("%s %s\n", line, m);
}

void comp(char *m, char *line)
{
    m[0]='\0';
    int ok_eq=0, ok_c=0;
    for(int i=0; i<(int)strlen(line); i++)
    {
        if(line[i]=='=')
            ok_eq=1;
        if(line[i]==';')
            ok_c=1;
    }
    char c[1024][1024];
    char line_copy[strlen(line)+1];
    strcpy(line_copy, line);
    char *p=strtok(line_copy, "=;");
    int n=0;
    while(p!=NULL)
    {
        strcpy(c[n], p);
        n++;
        p=strtok(NULL, "=;");
    }
    if(ok_eq==1 && ok_c==0)
    {
        c[1][strlen(c[1])-1]='\0';
        strcpy(m, c[1]);
    }
    if(ok_eq==0 && ok_c==1)
        strcpy(m, c[0]);
    if(ok_eq==1 && ok_c==1)
        strcpy(m, c[1]);
    delete_newline(m);
}

void jump(char *m, char *line)
{
    int ok=0;
    int k=0;
   for(int i=0; i<(int)strlen(line); i++)
   {
        if(ok==1)
            m[k++]=line[i];
        if(line[i]==';')
            ok=1;
   }
   if(ok==0)
    strcpy(m, "null");
}

command_type commandType(char *line)
{
    //A_COMMAND
    if(line[0]=='@')
        return A_COMMAND;
    //L_COMMAND
    if(line[0]=='(')
        return L_COMMAND;
    //C_COMMAND
    return C_COMMAND;
}

uint8_t destb(char *mnd)
{
    uint8_t bd=0x0;
    delete_newline(mnd);
    if(strcmp("null", mnd)==0)
    {
        bd=0;
    }
    else if(strcmp("M", mnd)==0)
    {
        bd=bd|(1<<0);
    }
    else if(strcmp("D", mnd)==0)
    {
        bd=bd|(1<<1);
    }
    else if(strcmp("MD", mnd)==0)
    {
        bd=bd|(1<<0);
        bd=bd|(1<<1);
    }
    else if(strcmp("A", mnd)==0)
    {
        bd=bd|(1<<2);
    }
    else if(strcmp("AM", mnd)==0)
    {
        bd=bd|(1<<0);
        bd=bd|(1<<2);
    }
    else if(strcmp("AD", mnd)==0)
    {
        bd=bd|(1<<1);
        bd=bd|(1<<2);
    }
    else if(strcmp("AMD", mnd)==0)
    {
        bd=bd|(1<<0);
        bd=bd|(1<<1);
        bd=bd|(1<<2);
    }
    return bd;
}

uint8_t compb(char *mnc)
{
    uint8_t bc=0x0;
    delete_newline(mnc);
    if(strchr(mnc, 'M')!=NULL)
        bc=bc|(1<<6);
    if(strcmp("0", mnc)==0)
    {
        bc=bc|(1<<1);
        bc=bc|(1<<3);
        bc=bc|(1<<5);
    }
    else if(strcmp("1", mnc)==0)
    {
       bc=bc|(1<<0);
       bc=bc|(1<<1);
       bc=bc|(1<<2);
       bc=bc|(1<<3); 
       bc=bc|(1<<4);
       bc=bc|(1<<5);
    }
    else if(strcmp("-1", mnc)==0)
    {
        bc=bc|(1<<1);
        bc=bc|(1<<3);
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("D", mnc)==0)
    {
        bc=bc|(1<<2);
        bc=bc|(1<<3);
    }
    else if(strcmp("A", mnc)==0 || strcmp("M", mnc)==0)
    {
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("!D", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<2);
        bc=bc|(1<<3);
    }
    else if(strcmp("!A", mnc)==0 || strcmp("!M", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("-D", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<2);
        bc=bc|(1<<3);
    }
    else if(strcmp("-A", mnc)==0 || strcmp("-M", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("D+1", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<2);
        bc=bc|(1<<3);
        bc=bc|(1<<4);
    }
    else if(strcmp("A+1", mnc)==0 || strcmp("M+1", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<2);
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("D-1", mnc)==0)
    {
        bc=bc|(1<<1);
        bc=bc|(1<<2);
        bc=bc|(1<<3);
    }
    else if(strcmp("A-1", mnc)==0 || strcmp("M-1", mnc)==0)
    {
        bc=bc|(1<<1);
        bc=bc|(1<<4);
        bc=bc|(1<<5);
    }
    else if(strcmp("D+A", mnc)==0 || strcmp("D+M", mnc)==0)
    {
        bc=bc|(1<<1);
    }
    else if(strcmp("D-A", mnc)==0 || strcmp("D-M", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<4);
    }
    else if(strcmp("A-D", mnc)==0 || strcmp("M-D", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<1);
        bc=bc|(1<<2);
    }
    else if(strcmp("D&A", mnc)==0 || strcmp("D&M", mnc)==0)
    {

    }
    else if(strcmp("D|A", mnc)==0 || strcmp("D|M", mnc)==0)
    {
        bc=bc|(1<<0);
        bc=bc|(1<<2);
        bc=bc|(1<<4);
    }

    return bc;
}

uint8_t jumpb(char *mnj)
{
    uint8_t bj=0x0;
    delete_newline(mnj);
    //printf("%s\n", mnj);
    if(strcmp("null", mnj)==0)
    {
        bj=0;
    }
    if(strcmp("JGT", mnj)==0)
    {
        bj=bj|(1<<0);
    }
    if(strcmp("JEQ", mnj)==0)
    {
        bj=bj|(1<<1);
    }
    if(strcmp("JGE", mnj)==0)
    {
        bj=bj|(1<<0);
        bj=bj|(1<<1);
    }
    if(strcmp("JLT", mnj)==0)
    {
        bj=bj|(1<<2);
    }
    if(strcmp("JNE", mnj)==0)
    {
        bj=bj|(1<<0);
        bj=bj|(1<<2);
    }
    if(strcmp("JLE", mnj)==0)
    {
        bj=bj|(1<<1);
        bj=bj|(1<<2);
    }
    if(strcmp("JMP", mnj)==0)
    {
        bj=bj|(1<<0);
        bj=bj|(1<<1);
        bj=bj|(1<<2);
    }
    /*for(int i=2; i>=0; i--)
        printf("%d", (bj>>i)&1);
    printf("\n");*/
    return bj;
}

uint16_t valueb(char *sb)
{
    uint16_t vb=0x0;
    int dec=strtol(sb, NULL, 10);
    //printf("%d\n", dec);
    int i=0;
    while(dec!=0)
    {
        if(dec%2==1)
            vb=vb|(1<<i);
        dec=dec/2;
        i++;
    }
    return vb;
}

symbol_entry *Constructor()
{
    symbol_entry *SymbolTable=NULL;
    hash_size=23;
    SymbolTable=(symbol_entry*)malloc(hash_size*sizeof(symbol_entry));
    if(SymbolTable==NULL)
    {
        fprintf(stderr, "Memory error for SymbolTable\n");
        exit(EXIT_FAILURE);
    }
    //SP
    SymbolTable[0].address=0;
    strcpy(SymbolTable[0].symbol, "SP");
    //LCL
    SymbolTable[1].address=1;
    strcpy(SymbolTable[1].symbol, "LCL");
    //ARG
    SymbolTable[2].address=2;
    strcpy(SymbolTable[2].symbol, "ARG");
    //THIS
    SymbolTable[3].address=3;
    strcpy(SymbolTable[3].symbol, "THIS");
    //THAT
    SymbolTable[4].address=4;
    strcpy(SymbolTable[4].symbol, "THAT");
    //R0
    SymbolTable[5].address=0;
    strcpy(SymbolTable[5].symbol, "R0");
    //R1
    SymbolTable[6].address=1;
    strcpy(SymbolTable[6].symbol, "R1");
    //R2
    SymbolTable[7].address=2;
    strcpy(SymbolTable[7].symbol, "R2");
    //R3
    SymbolTable[8].address=3;
    strcpy(SymbolTable[8].symbol, "R3");
    //R4
    SymbolTable[9].address=4;
    strcpy(SymbolTable[9].symbol, "R4");
    //R5
    SymbolTable[10].address=5;
    strcpy(SymbolTable[10].symbol, "R5");
    //R6
    SymbolTable[11].address=6;
    strcpy(SymbolTable[11].symbol, "R6");
    //R7
    SymbolTable[12].address=7;
    strcpy(SymbolTable[12].symbol, "R7");
    //R8
    SymbolTable[13].address=8;
    strcpy(SymbolTable[13].symbol, "R8");
    //R9
    SymbolTable[14].address=9;
    strcpy(SymbolTable[14].symbol, "R9");
    //R10
    SymbolTable[15].address=10;
    strcpy(SymbolTable[15].symbol, "R10");
    //R11
    SymbolTable[16].address=11;
    strcpy(SymbolTable[16].symbol, "R11");
    //R12
    SymbolTable[17].address=12;
    strcpy(SymbolTable[17].symbol, "R12");
    //R13
    SymbolTable[18].address=13;
    strcpy(SymbolTable[18].symbol, "R13");
    //R14
    SymbolTable[19].address=14;
    strcpy(SymbolTable[19].symbol, "R14");
    //R15
    SymbolTable[20].address=15;
    strcpy(SymbolTable[20].symbol, "R15");
    //SCREEN
    SymbolTable[21].address=16384;
    strcpy(SymbolTable[21].symbol, "SCREEN");
    //KBD
    SymbolTable[22].address=24576;
    strcpy(SymbolTable[22].symbol, "KBD");
    
    symbol_index=23;

    return SymbolTable;
}

bool contains(symbol_entry *SymbolTable, char *symbol)
{
    for(int i=0; i<hash_size; i++)
        if(strcmp(symbol, SymbolTable[i].symbol)==0)
            return true;
    return false;
}

symbol_entry *addEntry(symbol_entry *SymbolTable, char *symbol, int address)
{
    if(contains(SymbolTable, symbol)==true)
    {
        return SymbolTable;
    }
    else
    {
        symbol_entry *temp=NULL;
        if(symbol_index>=hash_size)
        {
            hash_size=symbol_index+1;
            temp=(symbol_entry*)realloc(SymbolTable, hash_size*sizeof(symbol_entry));
            if(temp==NULL)
            {
                fprintf(stderr, "Memory error for SymbolTable\n");
                exit(EXIT_FAILURE);
            }
            SymbolTable=temp;
        }
        //printf("Adding symbol %s with address %d to symbol table\n", symbol, address);
        SymbolTable[symbol_index].address=address;
        strcpy(SymbolTable[symbol_index].symbol, symbol);
        //printf("Added symbol %s with address %d to symbol table\n", SymbolTable[symbol_index].symbol, SymbolTable[symbol_index].address);
        symbol_index++;
        return SymbolTable;
    }
}

int GetAddress(symbol_entry *SymbolTable, char *symbol)
{
    for(int i=0; i<hash_size; i++)
    {
        //printf("%s 1 %s 2\n", symbol, SymbolTable[i].symbol);
        if(strcmp(symbol, SymbolTable[i].symbol)==0)
            return SymbolTable[i].address;
    }
    return -1;
}

void write_hack(char *filename, int size)
{
    FILE *f=NULL;
    char filename_hack[100];
    char *p=strtok(filename, ".");
    strcpy(filename_hack, p);
    strcat(filename_hack, ".hack");
    
    if((f=fopen(filename_hack, "w"))==NULL)
    {
        fprintf(stderr, "error opening .hack file\n");
        exit(EXIT_FAILURE);
    }

    //use only one of the following print modules:

    //copy-paste of the .asm file:
    /*for(int i=0; i<size; i++)
    {
        fprintf(f, "%s", asmb[i]);
    }*/
   
   //Parser:
   /*char sb[1024];
   char mnd[8];
   char mnc[32];
   char mnj[8];
    for(int i=0; i<size; i++)
    {
        if(commandType(asmb[i])==A_COMMAND)
        {
            symbol(sb, asmb[i], A_COMMAND);
            delete_newline(sb);
            fprintf(f, "commandType: A_COMMAND, symbol: %s\n", sb);
        }
        else if(commandType(asmb[i])==C_COMMAND)
        {
            dest(mnd, asmb[i]);
            comp(mnc, asmb[i]);
            jump(mnj, asmb[i]);
            delete_newline(mnd);
            delete_newline(mnc);
            delete_newline(mnj);
            fprintf(f, "commandType: C_COMMAND, dest: %s, comp: %s, jump: %s\n", mnd, mnc, mnj);
        }
        else if(commandType(asmb[i])==L_COMMAND)
        {
            symbol(sb, asmb[i], L_COMMAND);
            delete_newline(sb);
            fprintf(f, "commandType: L_COMMAND, symbol: %s\n", sb);
        }
    }*/

   //Code (No Symbol variant):
   /*char sb[1024];
   uint16_t sbb;
   uint8_t bd;
   char mnd[8];
   uint8_t bj;
   char mnj[8];
   uint8_t bc;
   char mnc[32];
   for(int i=0; i<size; i++)
   {
        if(commandType(asmb[i])==A_COMMAND)
        {
            fprintf(f, "0");
            symbol(sb, asmb[i], A_COMMAND);
            delete_newline(sb);
            sbb=valueb(sb);
            for(int i=14; i>=0; i--)
                fprintf(f, "%d", (sbb>>i)&1);
            fprintf(f, "\n");
        }
        if(commandType(asmb[i])==C_COMMAND)
        {
            fprintf(f, "111");
            comp(mnc, asmb[i]);
            bc=compb(mnc);
            for(int i=6; i>=0; i--)
                fprintf(f, "%d", (bc>>i)&1);
            //fprintf(f, " ");
            dest(mnd, asmb[i]);
            bd=destb(mnd);
            for(int i=2; i>=0; i--)
                fprintf(f, "%d", (bd>>i)&1);
            //fprintf(f, " ");
            jump(mnj, asmb[i]);
            bj=jumpb(mnj);
            for(int i=2; i>=0; i--)
                fprintf(f, "%d", (bj>>i)&1);
            //fprintf(f, " %s", mnc);
            fprintf(f, "\n");
        }
   }*/

    //Symbol variant:
    //Initialization:
    //printf("INITIALIZATION:\n");
    symbol_entry *SymbolTable=Constructor();
    /*for(int i=0; i<hash_size; i++)
        printf("%s %d\n", SymbolTable[i].symbol, SymbolTable[i].address);*/
    printf("initialization done\n");
    //First Pass:
    //printf("FIRST PASS:\n");
    char sb[1024];
    address_count=0;
    for(int i=0; i<size; i++)
    {
        if(commandType(asmb[i])==L_COMMAND)
        {
            symbol(sb, asmb[i], L_COMMAND);
            delete_newline(sb);
            //printf("%s %d\n", sb, address_count);
            //printf("SYMBOL INDEX: %d\n", symbol_index);
            SymbolTable=addEntry(SymbolTable, sb, address_count);
            //printf("%s %d\n", SymbolTable[address_count].symbol, SymbolTable[address_count].address);
            //printf("hash_size: %d\n", hash_size);
            /*for(int j=0; j<hash_size; j++)
                printf("%s %d\n", SymbolTable[j].symbol, SymbolTable[j].address);*/
        }
        else if(commandType(asmb[i])==C_COMMAND || commandType(asmb[i])==A_COMMAND)
        {
            address_count++;
        }
    }
    //printf("afterfp\n");
    /*for(int i=0; i<hash_size; i++)
        printf("%s %d\n", SymbolTable[i].symbol, SymbolTable[i].address);*/
    printf("first pass done\n");
    //Second Pass:
    address_count=16;
    uint16_t sbb;
    uint8_t bd;
    char mnd[8];
    uint8_t bj;
    char mnj[8];
    uint8_t bc;
    char mnc[32];
    for(int i=0; i<size; i++)
    {
        if(commandType(asmb[i])==C_COMMAND)
        {
            //printf("%s C_COMMAND\n", asmb[i]);
            fprintf(f, "111");
            comp(mnc, asmb[i]);
            bc=compb(mnc);
            for(int i=6; i>=0; i--)
                fprintf(f, "%d", (bc>>i)&1);
            //fprintf(f, " ");
            dest(mnd, asmb[i]);
            //printf("%s\n", mnd);
            bd=destb(mnd);
            for(int i=2; i>=0; i--)
                fprintf(f, "%d", (bd>>i)&1);
            //fprintf(f, " ");
            jump(mnj, asmb[i]);
            //printf("%s\n", mnj);
            bj=jumpb(mnj);
            for(int i=2; i>=0; i--)
                fprintf(f, "%d", (bj>>i)&1);
            //fprintf(f, " %s", mnc);
            fprintf(f, "\n");
        }
        else if(commandType(asmb[i])==A_COMMAND)
        {
            //printf("%s A_COMMAND\n", asmb[i]);
            symbol(sb, asmb[i], A_COMMAND);
            delete_newline(sb);
            //printf("%s is_symbol: %d\n", sb, is_symbol(sb));
            if(is_symbol(sb)==false)
            {
                fprintf(f, "0");
                sbb=valueb(sb);
                for(int i=14; i>=0; i--)
                    fprintf(f, "%d", (sbb>>i)&1);
                fprintf(f, "\n");
            }
            else
            {
                //printf("contained in symbol table: %d\n", contains(SymbolTable, sb));
                if(contains(SymbolTable, sb)==true)
                {
                    //printf("%d\n", GetAddress(SymbolTable, sb));
                    snprintf(sb, sizeof(sb), "%d", GetAddress(SymbolTable, sb));
                    fprintf(f, "0");
                    sbb=valueb(sb);
                    for(int i=14; i>=0; i--)
                        fprintf(f, "%d", (sbb>>i)&1);
                    fprintf(f, "\n");
                }
                else
                {
                    addEntry(SymbolTable, sb, address_count);
                    snprintf(sb, sizeof(sb), "%d", GetAddress(SymbolTable, sb));
                    fprintf(f, "0");
                    sbb=valueb(sb);
                    for(int i=14; i>=0; i--)
                        fprintf(f, "%d", (sbb>>i)&1);
                    fprintf(f, "\n");
                    address_count++;
                }
            }
        }
    }

    printf("second pass done\n.hack file written successfully\n");

    if(fclose(f)!=0)
    {
        fprintf(stderr, "error closing .hack file\n");
        exit(EXIT_FAILURE);
    }
    free(SymbolTable);
}

int correct_input_format(char *filename)
{
    if(strlen(filename)-1>100)
    {
        fprintf(stderr, ".asm file name too large\n");
        return 0;
    }
    int ok=0;
    for(int i=0; i<(int)strlen(filename); i++)
    {
        if(filename[i]=='.')
            ok=1;
    }
    if(ok==0)
    {
        fprintf(stderr, "invalid file format\n");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    printf("~~~Luca's Assembler for Hack~~~\n!!! Line size must be max 1024 characters\n!!! Do not include any whitespaces\n");
    int asmb_size;
    if(correct_input_format(argv[1])==0)
    {
        exit(EXIT_FAILURE);
    }
    asmb_size=open_asm(argv[1]);
    write_hack(argv[1], asmb_size);
    return 0;
}