#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define CHUNK 64

char **vm=NULL;
int lines=0;
int line_size=0;

typedef enum{
    C_ARITHMETIC,
    C_PUSH,
    C_POP,
    C_LABEL,
    C_GOTO,
    C_IF,
    C_FUNCTION,
    C_RETURN,
    C_CALL,
    null
}command_type;

char output_filename[1024];
char input_filename_global[1024];
char current_input_filename[1024];
char current_function_name[1024];

int isDirectory(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode); //1 - directory, 0 - not directory
}

int isVMfile(const char *filename)
{
    if(strstr(filename, ".vm")!=NULL)
        return 1;
    return 0;
}

void removeWhitespace(char *line)
{
    //trailing whitespace
    int i=strlen(line)-1;
    while(i>=0 && isspace(line[i])) 
    {
        i--;
    }
    line[i+1]='\0';
    //printf("%s\n", line);
    //leading whitespace
    int j=0;
    while(line[j]!='\0' && isspace(line[j]))
    {
        j++;
    } 
    if(j>0)
    {
        memmove(line, line+j, strlen(line+j)+1);
    }
    //printf("%s\n", line);
}

int isBlankLine(const char *line)
{
    for(int i=0; i<(int)strlen(line); i++)
        if(!isspace(line[i]))
            return 0;
    return 1;
}

char *comment(char *line)
{
    char *aux=(char*)malloc((strlen(line)+strlen("//")+strlen("\n")+1)*sizeof(char));
    if(aux==NULL)
    {
        fprintf(stderr, "(comment) error: memory allocation\n");
        exit(EXIT_FAILURE);
    }
    strcpy(aux, "//");
    strcat(aux, line);
    strcat(aux, "\n");
    return aux;
}

command_type commandType(char *line)
{
    char check_push[5];
    strncpy(check_push, line, 4);
    check_push[4]='\0';
    char check_pop[4];
    strncpy(check_pop, line, 3);
    check_pop[3]='\0';
    char check_label[6];
    strncpy(check_label, line, 5);
    check_label[5]='\0';
    char check_goto[5];
    strncpy(check_goto, line, 4);
    check_goto[4]='\0';
    char check_if[8];
    strncpy(check_if, line, 7);
    check_if[7]='\0';
    char check_function[9];
    strncpy(check_function, line, 8);
    check_function[8]='\0';
    char check_call[5];
    strncpy(check_call, line, 4);
    check_call[4]='\0';
    char check_return[7];
    strncpy(check_return, line, 6);
    check_return[6]='\0';


    if(strcmp(line, "add")==0 ||
    strcmp(line, "sub")==0 ||
    strcmp(line, "neg")==0 ||
    strcmp(line, "eq")==0 ||
    strcmp(line, "gt")==0 ||
    strcmp(line, "lt")==0 ||
    strcmp(line, "and")==0 ||
    strcmp(line, "or")==0 ||
    strcmp(line, "not")==0)
        return C_ARITHMETIC;
    else if(strcmp(check_push, "push")==0)
        return C_PUSH;
    else if(strcmp(check_pop, "pop")==0)
        return C_POP;
    else if(strcmp(check_label, "label")==0)
        return C_LABEL;
    else if(strcmp(check_goto, "goto")==0)
        return C_GOTO;
    else if(strcmp(check_if, "if-goto")==0)
        return C_IF;
    else if(strcmp(check_function, "function")==0)
        return C_FUNCTION;
    else if(strcmp(check_call, "call")==0)
        return C_CALL;
    else if(strcmp(check_return, "return")==0)
        return C_RETURN;
    return null;
}

char *arg1(char *line)
{
    if(commandType(line)==C_ARITHMETIC)
        {
            char *aux=(char*)malloc((strlen(line)+1)*sizeof(char));
            strcpy(aux, line);
            return aux;
        }
    else if(commandType(line)!=C_RETURN) //should not be called if the current command is C_RETURN
    {
        char *aux=(char*)malloc((strlen(line)+1)*sizeof(char));
        if(aux==NULL)
        {
            fprintf(stderr, "(arg1) error: memory allocation\n");
            exit(EXIT_FAILURE);
        }
        strcpy(aux, line);
        char *p=strtok(aux, " ");
        if(p!=NULL)
        {
            p=strtok(NULL, " ");
            char *arg=(char*)malloc((strlen(p)+1)*sizeof(char));
            if(arg==NULL)
            {
                fprintf(stderr, "(arg1) error: memory allocation\n");
                free(aux);
                exit(EXIT_FAILURE);
            }
            strcpy(arg, p);
            free(aux);
            return arg;
        }
    }
    return NULL;
}

int arg2(char *line)
{
    //should be called only if current command is C_PUSH, C_POP, C_FUNCTION, or C_CALL; any exceptions from this will lead to unexpected behavior
    char *aux=(char*)malloc((strlen(line)+1)*sizeof(char));
    if(aux==NULL)
    {
        fprintf(stderr, "(arg2) error: memory allocation\n");
        exit(EXIT_FAILURE);
    }
    strcpy(aux, line);
    char *p=strtok(aux, " ");
    if(p!=NULL)
    {
        p=strtok(NULL, " ");
        if(p!=NULL)
        {
            p=strtok(NULL, " ");
            int arg=strtol(p, NULL, 10);
            free(aux);
            return arg;
        }
    }
    return NULL;
}

void openVMfile(const char *input_filename) //or path
{
    FILE *f=NULL;
    f=fopen(input_filename, "r");
    if(f==NULL)
    {
        fprintf(stderr, "(openVMfile) error: opening file\n");
        exit(EXIT_FAILURE);
    }

    char *line=NULL;
    size_t bufsize=0;
    int filename_set=0;
    while((line_size=getline(&line, &bufsize, f))!=-1)
    {
        removeWhitespace(line);
        if(isBlankLine(line)==1)
            continue;
        if(line[0]=='/' && line[1]=='/')
            continue;
        if(lines==0 || lines%CHUNK==0)
        {
            char **temp=NULL;
            temp=(char**)realloc(vm, (lines+CHUNK)*sizeof(char*));
            if(temp==NULL)
            {
                for(int i=0; i<lines; i++)
                    free(vm[i]);
                free(vm);
                fprintf(stderr, "(openVMfile) error: memory allocation\n");
                exit(EXIT_FAILURE);
            }
            vm=temp;
        }
        if(filename_set==0)
        {
            int filename_size=strlen(input_filename_global)+1;
            vm[lines]=(char*)malloc(filename_size*sizeof(char));
            if(vm[lines]==NULL)
            {
                fprintf(stderr, "(openVMfile) error: memory allocation\n");
                exit(EXIT_FAILURE);
            }
            strcpy(vm[lines], input_filename_global);
            removeWhitespace(vm[lines]);
            //printf("%s\n", vm[lines]);
            lines++;
            filename_set=1;
        }
        int trimmed_line_size=strlen(line)+1;
        vm[lines]=(char*)malloc(trimmed_line_size*sizeof(char));
        if(vm[lines]==NULL)
        {
            fprintf(stderr, "(openVMfile) error: memory allocation\n");
            exit(EXIT_FAILURE);
        }
        strcpy(vm[lines], line);
        line[0]='\0';
        lines++;
    }
    free(line);
    printf("input file read successfully\n");

    if(fclose(f)!=0)
    {
        fprintf(stderr, "(openVMfile) error: closing file\n");
        exit(EXIT_FAILURE);
    }
}

void setFileName(char *filename, int is_directory)
{
    if(is_directory==0)
    {
        strncpy(output_filename, filename, strlen(filename)-3);
        strcat(output_filename, ".asm");
    }
    else if(is_directory==1)
    {
        strcpy(output_filename, filename);
        strcat(output_filename, ".asm");
    }
    printf("output file name: %s\n", output_filename);
}

void openVM(const char *path)
{
    //this function takes place for the following routines: hasMoreCommands, advance
    if(isDirectory(path)==1)
    {
        printf("argument is directory\n");
        char modifiable_path[1024];
        strcpy(modifiable_path, path);
        setFileName(modifiable_path, 1);
        DIR *directory;
        struct dirent *entry;
        directory=opendir(path);
        if(directory==NULL)
        {
            fprintf(stderr, "(openVM) error: opening directory\n");
            exit(EXIT_FAILURE);
        }
        while((entry=readdir(directory))!=NULL)
        {
            if(isVMfile(entry->d_name)==1)
            {
                char filepath[1024]="\0";
                strcat(filepath, "./");
                strcat(filepath, path);
                strcat(filepath, "/");
                printf(".vm file found: %s\n", entry->d_name);
                strcpy(input_filename_global, entry->d_name);
                //printf("%s\n", input_filename_global);
                strcat(filepath, entry->d_name);
                openVMfile(filepath);
            }
        }
        if(closedir(directory)==-1)
        {
            fprintf(stderr, "(openVM) error: closing directory\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if(isVMfile(path)==0)
        {
            fprintf(stderr, "(openVM) error: .vm extension is mandatory\n");
            exit(EXIT_FAILURE);
        }
        else
        {
             printf("argument is file\n");
             strcpy(input_filename_global, path);
             //printf("%s\n", input_filename_global);
             char modifiable_path[1024];
             strcpy(modifiable_path, path);
             setFileName(modifiable_path, 0);
             openVMfile(path);
        }
    }
}

void writeCall(char *functionName, int numArgs, FILE **of)
{
    printf("function called: %s\t number of arguments: %d\n", functionName, numArgs);
    static int ret_id=0;

    fprintf(*of, "@%s$ret.%d\n", current_function_name, ret_id);
    fprintf(*of, "D=A\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M+1\n");

    fprintf(*of, "@LCL\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M+1\n");

    fprintf(*of, "@ARG\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M+1\n");

    fprintf(*of, "@THIS\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M+1\n");

    fprintf(*of, "@THAT\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M+1\n");

    fprintf(*of, "@SP\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@5\n");
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "@%d\n", numArgs);
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "@ARG\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@SP\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@LCL\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@%s\n", functionName);
    fprintf(*of, "0;JMP\n");

    fprintf(*of, "(%s$ret.%d)\n", current_function_name, ret_id);

    ret_id++;
}

void Constructor(FILE **of)
{
    *of=fopen(output_filename, "w");
    if(*of==NULL)
    {
        fprintf(stderr, "(Constructor) error: opening file\n");
        exit(EXIT_FAILURE);
    }
    //the following code substitutes the writeInit function:
    fprintf(*of, "//initialize the stack pointer to 256\n");
    fprintf(*of, "@256\n");
    fprintf(*of, "D=A\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=D\n");
    //the default convention for the memory segments (if Sys.init is not used):
    /*fprintf(*of, "//initialize the LCL segment to 300\n");
    fprintf(*of, "@300\n");
    fprintf(*of, "D=A\n");
    fprintf(*of, "@LCL\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "//initialize the ARG segment to 400\n");
    fprintf(*of, "@400\n");
    fprintf(*of, "D=A\n"); 
    fprintf(*of, "@ARG\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "//initialize the THIS segment to 3000\n");
    fprintf(*of, "@3000\n");
    fprintf(*of, "D=A\n");
    fprintf(*of, "@THIS\n");
    fprintf(*of, "M=D\n");
    fprintf(*of, "//initialize the THAT segment to 3010\n");
    fprintf(*of, "@3010\n");
    fprintf(*of, "D=A\n");
    fprintf(*of, "@THAT\n");
    fprintf(*of, "M=D\n");*/
    //bootstrap:
    strcpy(current_function_name, "Sys.init");
    fprintf(*of, "//call Sys.init\n");
    writeCall(current_function_name, 0, of);
}

void Close(FILE *of)
{
    if(fclose(of)!=0)
    {
        fprintf(stderr, "(Close) error: closing file\n");
        exit(EXIT_FAILURE);
    }
}

void writeArithmetic(char *command, FILE **of)
{
    static long int label_count=0;
    if(strcmp(command, "add")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");
        fprintf(*of, "@R13\n");
        fprintf(*of, "M=D\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");
        fprintf(*of, "@R13\n");
        fprintf(*of, "D=D+M\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=D\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    else if(strcmp(command, "sub")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");
        fprintf(*of, "@R13\n");
        fprintf(*of, "M=D\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");
        fprintf(*of, "@R13\n");
        fprintf(*of, "D=D-M\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=D\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    else if(strcmp(command, "neg")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=-M\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    else if(strcmp(command, "eq")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=D-M\n");

        fprintf(*of, "@EQ_TRUE%ld\n", label_count);
        fprintf(*of, "D;JEQ\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=0\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
        fprintf(*of, "@END_EQ%ld\n", label_count);
        fprintf(*of, "0;JMP\n");

        fprintf(*of, "(EQ_TRUE%ld)\n", label_count);
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=-1\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");

        fprintf(*of, "(END_EQ%ld)\n", label_count);

        label_count++;
    }
    else if(strcmp(command, "gt")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M-D\n");

        fprintf(*of, "@GT_TRUE%ld\n", label_count);
        fprintf(*of, "D;JGT\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=0\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
        fprintf(*of, "@END_GT%ld\n", label_count);
        fprintf(*of, "0;JMP\n");

        fprintf(*of, "(GT_TRUE%ld)\n", label_count);
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=-1\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");

        fprintf(*of, "(END_GT)%ld\n", label_count);

        label_count++;
    }
    else if(strcmp(command, "lt")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M-D\n");

        fprintf(*of, "@LT_TRUE%ld\n", label_count);
        fprintf(*of, "D;JLT\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=0\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
        fprintf(*of, "@END_LT%ld\n", label_count);
        fprintf(*of, "0;JMP\n");

        fprintf(*of, "(LT_TRUE%ld)\n", label_count);
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=-1\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");

        fprintf(*of, "(END_LT%ld)\n", label_count);

        label_count++;
    }
    else if(strcmp(command, "and")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=D&M\n");
        
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=D\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    else if(strcmp(command, "or")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "D=D|M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=D\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    else if(strcmp(command, "not")==0)
    {
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M-1\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=!M\n");

        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
}

void writePushPop(command_type command, char *segment, int index, FILE **of)
{
    if (command==C_PUSH) 
    {
        //fprintf(*of, "push %s %d\n", segment, index);
        if(strcmp(segment, "constant")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "local")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@LCL\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "A=D\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "argument")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@ARG\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "A=D\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "this")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@THIS\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "A=D\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "that")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@THAT\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "A=D\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "static")==0)
        {
            fprintf(*of, "@%s.%d\n", current_input_filename, index);
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "temp")==0)
        {
            fprintf(*of, "@R%d\n", index+5);
            fprintf(*of, "D=M\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M+1\n");
        }
        else if(strcmp(segment, "pointer")==0)
        {
            if(index==0)
            {
                fprintf(*of, "@THIS\n");
                fprintf(*of, "D=M\n");
                fprintf(*of, "@SP\n");
                fprintf(*of, "A=M\n");
                fprintf(*of, "M=D\n");
                fprintf(*of, "@SP\n");
                fprintf(*of, "M=M+1\n");
            }
            else if(index==1)
            {
                fprintf(*of, "@THAT\n");
                fprintf(*of, "D=M\n");
                fprintf(*of, "@SP\n");
                fprintf(*of, "A=M\n");
                fprintf(*of, "M=D\n");
                fprintf(*of, "@SP\n");
                fprintf(*of, "M=M+1\n"); 
            }
        }
    } 
    else if (command == C_POP) 
    {
        if(strcmp(segment, "local")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@LCL\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=D+A\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "argument")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@ARG\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "this")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@THIS\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "that")==0)
        {
            fprintf(*of, "@%d\n", index);
            fprintf(*of, "D=A\n");
            fprintf(*of, "@THAT\n");
            fprintf(*of, "D=D+M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "M=D\n");
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@R13\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "static")==0)
        {
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@%s.%d\n", current_input_filename, index);
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "temp")==0)
        {
            fprintf(*of, "@SP\n");
            fprintf(*of, "M=M-1\n");
            fprintf(*of, "A=M\n");
            fprintf(*of, "D=M\n");
            fprintf(*of, "@R%d\n", index+5);
            fprintf(*of, "M=D\n");
        }
        else if(strcmp(segment, "pointer")==0)
        {
            if(index==0)
            {
                fprintf(*of, "@SP\n");
                fprintf(*of, "M=M-1\n");
                fprintf(*of, "A=M\n");
                fprintf(*of, "D=M\n");
                fprintf(*of, "@THIS\n");
                fprintf(*of, "M=D\n");
            }
            else if(index==1)
            {
                fprintf(*of, "@SP\n");
                fprintf(*of, "M=M-1\n");
                fprintf(*of, "A=M\n");
                fprintf(*of, "D=M\n");
                fprintf(*of, "@THAT\n");
                fprintf(*of, "M=D\n");
            }
        }
    }
}

void writeLabel(char *label, FILE **of)
{
    fprintf(*of, "(%s$%s)\n", current_function_name, label);
}

void writeGoto(char *label, FILE **of)
{
    fprintf(*of, "@%s$%s\n", current_function_name, label);
    fprintf(*of, "0;JMP\n");
}

void writeIf(char *label, FILE **of)
{
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M-1\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@%s$%s\n", current_function_name, label);
    fprintf(*of, "D;JNE\n");
}

void writeReturn(FILE **of)
{
    fprintf(*of, "@LCL\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@R13\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@5\n");
    fprintf(*of, "D=A\n");
    fprintf(*of, "@R13\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "A=A-D\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@R14\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@SP\n");
    fprintf(*of, "M=M-1\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@ARG\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "M=D\n");
    
    fprintf(*of, "@ARG\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@SP\n");
    fprintf(*of, "M=D+1\n");
    
    fprintf(*of, "@R13\n"); //@R13 - endFrame, @R14 - retAddr
    fprintf(*of, "D=M\n");
    fprintf(*of, "@1\n");
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "A=D\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@THAT\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@R13\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@2\n");
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "A=D\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@THIS\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@R13\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@3\n");
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "A=D\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@ARG\n");
    fprintf(*of, "M=D\n");

    fprintf(*of, "@R13\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@4\n");
    fprintf(*of, "D=D-A\n");
    fprintf(*of, "A=D\n");
    fprintf(*of, "D=M\n");
    fprintf(*of, "@LCL\n"); 
    fprintf(*of, "M=D\n");

    fprintf(*of, "@R14\n");
    fprintf(*of, "A=M\n");
    fprintf(*of, "0;JMP\n");
}

void writeFunction(char *functionName, int numLocals, FILE **of)
{
    fprintf(*of, "(%s)\n", functionName);
    for(int i = 0; i < numLocals; i++)
    {
        fprintf(*of, "@0\n");
        fprintf(*of, "D=A\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "A=M\n");
        fprintf(*of, "M=D\n");
        fprintf(*of, "@SP\n");
        fprintf(*of, "M=M+1\n");
    }
    current_function_name[0]='\0';
    strcpy(current_function_name, functionName);
}

char *parseLabel(char *str)
{
    char *p = strtok(str, " ");
    p = strtok(NULL, " ");
    if(p != NULL)
        return p;
    return NULL;
}

int main(int argc, char **argv)
{
    printf("~~~ Luca's VM translator ~~~\n");
    printf("!!! If possible, do not include whitespaces/comments, it might not work fully\n");
    if(argc <= 1)
    {
        fprintf(stderr, "(main) error: not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    openVM(argv[1]);
    FILE *of=NULL;
    Constructor(&of);
    for(int i=0; i<lines; i++)
    {
        char *com=NULL;
        com=comment(vm[i]);
        fprintf(of, "%s", com);
        if(strstr(vm[i], ".vm")!=NULL)
        {
            strcpy(current_input_filename, vm[i]);
            char *p=strtok(current_input_filename, ".");
            if(p!=NULL)
            {
                strcpy(current_input_filename, p);
            }
        }
        //printf("%s %s\n", vm[i], current_input_filename);
        if(commandType(vm[i])==C_PUSH || commandType(vm[i])==C_POP)
        {
            writePushPop(commandType(vm[i]), arg1(vm[i]), arg2(vm[i]), &of);
        }
        else if(commandType(vm[i])==C_ARITHMETIC)
        {
            writeArithmetic(vm[i], &of);
        }
        else if(commandType(vm[i])==C_LABEL)
        {
            //printf("%s", parseLabel(vm[i]));
            writeLabel(parseLabel(vm[i]), &of);
        }
        else if(commandType(vm[i])==C_GOTO)
        {
            writeGoto(parseLabel(vm[i]), &of);
        }
        else if(commandType(vm[i])==C_IF)
        {
            writeIf(parseLabel(vm[i]), &of);
        }
        else if(commandType(vm[i])==C_CALL)
        {
            writeCall(arg1(vm[i]), arg2(vm[i]), &of);
        }
        else if(commandType(vm[i])==C_RETURN)
        {
            writeReturn(&of);
        }
        else if(commandType(vm[i])==C_FUNCTION)
        {
            writeFunction(arg1(vm[i]), arg2(vm[i]), &of);
        }
    }
    Close(of);
    printf("output file written successfully\n");

    //testing/debugging:
    /*for(int i=0; i<lines; i++)
        {
           //printf("%s\n", vm[i]);
           char *com=NULL;
           char *a1=NULL;
           int a2;

           com=comment(vm[i]);
           printf("comment: %s", com);
           free(com);

           printf("commandType: ");
           if(commandType(vm[i])==C_ARITHMETIC)
                printf("C_ARITHMETIC\n");
            else if(commandType(vm[i])==C_CALL)
                printf("C_CALL\n");
            else if(commandType(vm[i])==C_FUNCTION)
                printf("C_FUNCTION\n");
            else if(commandType(vm[i])==C_GOTO)
                printf("C_GOTO\n");
            else if(commandType(vm[i])==C_IF)
                printf("C_IF\n");
            else if(commandType(vm[i])==C_LABEL)
                printf("C_LABEL\n");
            else if(commandType(vm[i])==C_POP)
                printf("C_POP\n");
            else if(commandType(vm[i])==C_PUSH)
                printf("C_PUSH\n");
            else if(commandType(vm[i])==C_RETURN)
                printf("C_RETURN\n");

            if(commandType(vm[i])!=C_RETURN)
            {
                a1=arg1(vm[i]);
                printf("arg1: %s\n", a1);
                free(a1);
            }

            if(commandType(vm[i])==C_PUSH || commandType(vm[i])==C_POP || commandType(vm[i])==C_FUNCTION || commandType(vm[i])==C_CALL)
            {
                a2=arg2(vm[i]);
                printf("arg2: %d\n", a2);
            }

            printf("\n");
        }*/
    /*for(int i=0; i<lines; i++)
        printf("%s\n", vm[i]);*/
    for(int i=0; i<lines; i++)
        free(vm[i]);
    free(vm);
    return 0;
}