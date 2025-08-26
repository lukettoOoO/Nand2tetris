#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

#define CHUNK 32
#define MAX_HACK_SIZE 32768
#define KEY_COUNT 21

FILE *inputFile;
char *currentLine = NULL; //current line from the contents of the input file/files
char *inputStream = NULL; //contents of the input file/files
int inputSize = 0; //size of the current input file
char currentToken[MAX_HACK_SIZE];
int currentTokenIndex = 0;
int currentCompileTokenIndex = 0;
int indentLevel = 0;

int varCount = 0;
int staticCount = 0;
int fieldCount = 0;
int argCount = 0;
int classTableIndex = 0;
int subroutineTableIndex = 0;

char *symbolList = "{}()[].,;+-*/&|<>=~";
char *keywordList[] = {"class", "constructor", "function",
    "method", "field", "static", "var",
    "int", "char", "boolean", "void", "true",
    "false", "null", "this", "let", "do",
    "if", "else", "while", "return"};

typedef enum {
    KEYWORD, //0
    SYMBOL, //1
    IDENTIFIER, //2
    INT_CONST, //3
    STRING_CONST, //4
}token_type;

typedef enum {
    CLASS,
    METHOD,
    FUNCTION,
    CONSTRUCTOR,
    INT,
    BOOLEAN,
    CHAR,
    VOID,
    VAR,
    STATIC,
    FIELD,
    LET,
    DO,
    IF,
    ELSE,
    WHILE,
    RETURN,
    TRUE,
    FALSE,
    NULL_KEY,
    THIS,
    UNKNOWN
}key_type;

typedef enum {
    STATIC_SYMBOL,
    FIELD_SYMBOL,
    ARG_SYMBOL,
    VAR_SYMBOL,
    NONE_SYMBOL
}identifier_kind;

typedef enum {
    CONST_SEGMENT,
    ARG_SEGMENT,
    LOCAL_SEGMENT,
    STATIC_SEGMENT,
    THIS_SEGMENT,
    THAT_SEGMENT,
    POINTER_SEGMENT,
    TEMP_SEGMENT
}vm_segment;

typedef enum {
    ADD_COMMAND,
    SUB_COMMAND,
    NEG_COMMAND,
    EQ_COMMAND,
    GT_COMMAND,
    LT_COMMAND,
    AND_COMMAND,
    OR_COMMAND,
    NOT_COMMAND
}vm_command;

typedef struct {
    char name[CHUNK];
    char type[CHUNK];
    identifier_kind kind;
    int index;
}Symbol;
Symbol classTable[MAX_HACK_SIZE];
Symbol subroutineTable[MAX_HACK_SIZE];

void removeCharacter(int pos)
{
    for(int i = pos; i < inputSize; i++)
    {
        inputStream[i] = inputStream[i+1];
    }
    inputSize--;
    inputStream[inputSize] = '\0';
}

char* ToUpper(char* str)
{
    int len = strlen(str);
    for(int i = 0; i < len; i++)
        str[i] = toupper(str[i]);
    return str;
}

//JackCompiler:
//The SymbolTable module:

void SymbolTableConstructor() //Constructor
{   //Creates a new empty symbol table
    varCount = 0;
    staticCount = 0;
    fieldCount = 0;
    argCount = 0;
    classTableIndex = 0;
    subroutineTableIndex = 0;
    memset(classTable, 0, sizeof(classTable));
    memset(subroutineTable, 0, sizeof(subroutineTable));
    //printf("<<<NEW CLASS>>>\n");   
}

void startSubroutine()
{
    varCount = 0;
    argCount = 0;
    subroutineTableIndex = 0;
    memset(subroutineTable, 0, sizeof(subroutineTable));
    //printf("<<<NEW SUBROUTINE>>>\n");
}

int VarCount(identifier_kind kind);

Symbol Define(char *name, char *type, identifier_kind kind) //defines a new identifier of a given name, type, and kind and assigns it a running index. 
{                                                           //STATIC and FIELD identifiers have a class scope, while ARG and VAR identifiers have a subroutine scope
    Symbol newSymbol;
    memset(&newSymbol, 0, sizeof(Symbol));
    strcpy(newSymbol.name, name);
    strcpy(newSymbol.type, type);
    newSymbol.kind = kind;
    newSymbol.index = VarCount(kind);

    if (kind == STATIC_SYMBOL || kind == FIELD_SYMBOL) {
        classTable[classTableIndex++] = newSymbol;
    } else if (kind == ARG_SYMBOL || kind == VAR_SYMBOL) {
        subroutineTable[subroutineTableIndex++] = newSymbol;
    }

    /*printf("(DECLARATION) Name: %s, Type: %s, Kind: %d, Index: %d\n",
       newSymbol.name, newSymbol.type, newSymbol.kind, newSymbol.index);*/
    return newSymbol;
}

int VarCount(identifier_kind kind)
{
    switch (kind)
    {
    case STATIC_SYMBOL:
        return staticCount++;
        break;
    case FIELD_SYMBOL:
        return fieldCount++;
        break;
    case ARG_SYMBOL:
        return argCount++;
        break;
    case VAR_SYMBOL:
        return varCount++;
        break;
    default:
        return 0;
        break;
    }

}

identifier_kind KindOf(char* name)
{
    for(int i = 0; i < subroutineTableIndex; i++)
    {
        if(strcmp(subroutineTable[i].name, name) == 0)
            return subroutineTable[i].kind;
    }
    for(int i = 0; i < classTableIndex; i++)
    {
        if(strcmp(classTable[i].name, name) == 0)
            return classTable[i].kind;
    }   
    return NONE_SYMBOL;
}

char* TypeOf(char* name)
{
    for(int i = 0; i < subroutineTableIndex; i++)
    {
        if(strcmp(subroutineTable[i].name, name) == 0)
            return subroutineTable[i].type;
    }
    for(int i = 0; i < classTableIndex; i++)
    {
        if(strcmp(classTable[i].name, name) == 0)
            return classTable[i].type;
    }   
    return NULL;
}

int IndexOf(char* name)
{
    for(int i = 0; i < subroutineTableIndex; i++)
    {
        if(strcmp(subroutineTable[i].name, name) == 0)
            return subroutineTable[i].index;
    }
    for(int i = 0; i < classTableIndex; i++)
    {
        if(strcmp(classTable[i].name, name) == 0)
            return classTable[i].index;
    }   
    return -1;
}

Symbol* lookup(char *name)
{
    for(int i = 0; i < subroutineTableIndex; i++)
    {
        if(strcmp(subroutineTable[i].name, name) == 0)
            return &subroutineTable[i];
    }
    for(int i = 0; i < classTableIndex; i++)
    {
        if(strcmp(classTable[i].name, name) == 0)
            return &classTable[i];
    }
    return NULL;
}

//VMWriter module:

//Constructer done in analyzerLogic function

void writePush(FILE* outputVMFile, vm_segment Segment, int Index)
{
    switch(Segment)
    {
        case CONST_SEGMENT:
            fprintf(outputVMFile, "push constant %d\n", Index);
            break;
        case ARG_SEGMENT:
            fprintf(outputVMFile, "push argument %d\n", Index);
            break;
        case LOCAL_SEGMENT:
            fprintf(outputVMFile, "push local %d\n", Index);
            break;
        case STATIC_SEGMENT:
            fprintf(outputVMFile, "push static %d\n", Index);
            break;
        case THIS_SEGMENT:
            fprintf(outputVMFile, "push this %d\n", Index);
            break;
        case THAT_SEGMENT:
            fprintf(outputVMFile, "push that %d\n", Index);
            break;
        case POINTER_SEGMENT:
            fprintf(outputVMFile, "push pointer %d\n", Index);
            break;
        case TEMP_SEGMENT:
            fprintf(outputVMFile, "push temp %d\n", Index);
            break;
        default:
            fprintf(stderr, "(writePush): invalid segment\n");
            break;
    }
}

void writePop(FILE* outputVMFile, vm_segment Segment, int Index)
{
    switch(Segment)
    {
        case CONST_SEGMENT:
            fprintf(stderr, "(writePop): cannot pop into constant segment\n");
            break;
        case ARG_SEGMENT:
            fprintf(outputVMFile, "pop argument %d\n", Index);
            break;
        case LOCAL_SEGMENT:
            fprintf(outputVMFile, "pop local %d\n", Index);
            break;
        case STATIC_SEGMENT:
            fprintf(outputVMFile, "pop static %d\n", Index);
            break;
        case THIS_SEGMENT:
            fprintf(outputVMFile, "pop this %d\n", Index);
            break;
        case THAT_SEGMENT:
            fprintf(outputVMFile, "pop that %d\n", Index);
            break;
        case POINTER_SEGMENT:
            fprintf(outputVMFile, "pop pointer %d\n", Index);
            break;
        case TEMP_SEGMENT:
            fprintf(outputVMFile, "pop temp %d\n", Index);
            break;
        default:
            break;
    }
}

void WriteArithmetic(FILE* outputVMFile, vm_command command)
{
    switch(command)
    {
        case ADD_COMMAND:
            fprintf(outputVMFile, "add\n");
            break;
        case SUB_COMMAND:
            fprintf(outputVMFile, "sub\n");
            break;
        case NEG_COMMAND:
            fprintf(outputVMFile, "neg\n");
            break;
        case EQ_COMMAND:
            fprintf(outputVMFile, "eq\n");
            break;
        case GT_COMMAND:
            fprintf(outputVMFile, "gt\n");
            break;
        case LT_COMMAND:
            fprintf(outputVMFile, "lt\n");
            break;
        case AND_COMMAND:
            fprintf(outputVMFile, "and\n");
            break;
        case OR_COMMAND:
            fprintf(outputVMFile, "or\n");
            break;
        case NOT_COMMAND:
            fprintf(outputVMFile, "not\n");
            break;
        default:
            fprintf(stderr, "(writeArithmetic): invalid command\n");
            break;
    }
}

void WriteLabel(FILE* outputVMFile, char* label)
{
    fprintf(outputVMFile, "label %s\n", label);
}

void WriteGoto(FILE* outputVMFile, char* label)
{
    fprintf(outputVMFile, "goto %s\n", label);
}

void WriteIf(FILE* outputVMFile, char* label)
{
    fprintf(outputVMFile, "if-goto %s\n", label);
}

void writeCall(FILE* outputVMFile, char* name, int nArgs)
{
    fprintf(outputVMFile, "call %s %d\n", name, nArgs);
}

void writeFunction(FILE* outputVMFile, char* name, int nLocals)
{
    fprintf(outputVMFile, "function %s %d\n", name, nLocals);
}

void writeReturn(FILE* outputVMFile)
{
    fprintf(outputVMFile, "return\n");
}

//The JackTokenizer module:
//Constructor:

void Constructor(const char *inputName)
{
    //printf("CONSTRUCTOR INPUT NAME: %s\n", inputName);
    inputStream = NULL;
    inputSize = 0;
    inputFile = fopen(inputName, "r");
    if(inputFile == NULL)
    {
        fprintf(stderr, "(Constructor): error opening file\n");
        exit(EXIT_FAILURE);
    }

    char c = ' ';
    int currentSize = 0;
    inputStream = (char*)malloc(CHUNK * sizeof(char));
    currentSize = currentSize + CHUNK;
    if(inputStream == NULL)
    {
        fprintf(stderr, "(Constructor): error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    while((c = fgetc(inputFile)) != EOF)
    {
        if(inputSize == currentSize)
        {
            char *p = (char*)realloc(inputStream, (currentSize + CHUNK) * sizeof(char));
            currentSize = currentSize + CHUNK;
            if(p == NULL)
            {
                fprintf(stderr, "(Constructor): error reallocating memory\n");
                free(inputStream);
                exit(EXIT_FAILURE);
            }
            inputStream = p;
        }
        inputStream[inputSize++] = c;
    }

    for(int i = 0; i < inputSize - 1; i++)
    {
        //removing single comments
        if(inputStream[i] == '/' && inputStream[i+1] == '/')
        {
            while(inputStream[i] != '\n' && inputStream[i] != '\0')
            {
                removeCharacter(i);
            }
        }
        //removing API comments
        if(i < inputSize - 2 && inputStream[i] == '/' && inputStream[i+1] == '*' && inputStream[i+2] == '*')
        {
            removeCharacter(i);
            removeCharacter(i);
            removeCharacter(i);
            while(!(inputStream[i] == '*' && inputStream[i+1] == '/') && inputStream[i] != '\0')
            {
                removeCharacter(i);
            }
            removeCharacter(i);
            removeCharacter(i);
        }
        //removing comments until closing
        if(inputStream[i] == '/' && inputStream[i+1] == '*')
        {
            removeCharacter(i);
            removeCharacter(i);
            while(!(inputStream[i] == '*' && inputStream[i+1] == '/') && inputStream[i] != '\0')
            {
                removeCharacter(i);
            }
            removeCharacter(i);
            removeCharacter(i);
        }
    }
      
    //printf("%s", inputStream);
}

bool hasMoreTokens()
{
    if(currentLine == NULL)
        return false;
    return true;
}

void advance()
{
    currentLine = strtok(NULL, "\n\r\t\f\v");
}

token_type tokenType(char *token)
{
    if(strchr(token, '"') != NULL)
        return STRING_CONST;

    int isIntConst = true;
    for(int i = 0; i < strlen(token); i++)
    {
        if(!isdigit(token[i]))
            isIntConst = false;
    }
    if(isIntConst)
        return INT_CONST;

    if(strchr(symbolList, token[0]) != NULL)
        return SYMBOL;

    
    for(int i = 0; i < KEY_COUNT; i++)
    {
        if(strcmp(keywordList[i], token) == 0)
        {
            return KEYWORD;
        }
    }

    return IDENTIFIER;
}

key_type keyWord(char *key)
{
    if (strcmp(key, "class") == 0) return CLASS;
    if (strcmp(key, "method") == 0) return METHOD;
    if (strcmp(key, "function") == 0) return FUNCTION;
    if (strcmp(key, "constructor") == 0) return CONSTRUCTOR;
    if (strcmp(key, "int") == 0) return INT;
    if (strcmp(key, "boolean") == 0) return BOOLEAN;
    if (strcmp(key, "char") == 0) return CHAR;
    if (strcmp(key, "void") == 0) return VOID;
    if (strcmp(key, "var") == 0) return VAR;
    if (strcmp(key, "static") == 0) return STATIC;
    if (strcmp(key, "field") == 0) return FIELD;
    if (strcmp(key, "let") == 0) return LET;
    if (strcmp(key, "do") == 0) return DO;
    if (strcmp(key, "if") == 0) return IF;
    if (strcmp(key, "else") == 0) return ELSE;
    if (strcmp(key, "while") == 0) return WHILE;
    if (strcmp(key, "return") == 0) return RETURN;
    if (strcmp(key, "true") == 0) return TRUE;
    if (strcmp(key, "false") == 0) return FALSE;
    if (strcmp(key, "null") == 0) return NULL_KEY;
    if (strcmp(key, "this") == 0) return THIS;
    return UNKNOWN;

}

char symbol(char *sym)
{
    return sym[0];
}

char *identifier(char *ident)
{
    return ident;
}

int intVal(char *intv)
{
    return atoi(intv);
}

char *stringVal(char *str)
{
    if (strlen(str) < 2) 
        return strdup("");
    char *strval = malloc(strlen(str) - 1);
    if (!strval) 
        return NULL;
    for (int i = 1; i < strlen(str) - 1; i++)
    {
        strval[i - 1] = str[i];
    }
    strval[strlen(str) - 2] = '\0'; // Null-terminate the string
    return strval;
}

//main function for JackTokenizer:
void clearCurrentToken()
{
    memset(currentToken, 0, sizeof(currentToken));
    currentTokenIndex = 0;
}

void getIntegerConstant(int *i)
{
    while(isdigit(currentLine[*i]))
    {
        currentToken[currentTokenIndex++] = currentLine[*i];
        (*i)++;
    }
    currentToken[currentTokenIndex] = '\0';
}

void getStringConstant(int *i)
{
    currentToken[currentTokenIndex++] = currentLine[*i];
    (*i)++;
    while(currentLine[*i] != '"')
    {
        currentToken[currentTokenIndex++] = currentLine[*i];
        (*i)++;
    }
    currentToken[currentTokenIndex++] = currentLine[*i];
    currentToken[currentTokenIndex] = '\0';
}

void getSymbol(int *i)
{
    char *found = strchr(symbolList, currentLine[*i]);
    currentToken[currentTokenIndex++] = found[0];
    currentToken[currentTokenIndex] = '\0';
    //printf("DEBUG: Symbol at index %d: '%c'\n", i, found[0]);
}

void getKeywordOrIdentifier(int *i)
{
    while(isalnum(currentLine[*i]) || currentLine[*i] == '_')
    {
        currentToken[currentTokenIndex++] = currentLine[*i];
        (*i)++;
    }
    (*i)--;
    currentToken[currentTokenIndex] = '\0';
}

char *tokenAlloc(char **token, int tokenSize, int size)
{
    char *t = (char*)malloc(size * sizeof(char));
    if(t == NULL)
    {
        fprintf(stderr, "(tokenAlloc): Memory allocation error\n");
        for(int i = 0; i < tokenSize; i++)
        {
            if(token[i] != NULL)
                free(token[i]);
        }
        if(token != NULL)
            free(token);
        exit(EXIT_FAILURE);
    }
    return t;
}

char **JackTokenizer(const char *inputName, int *tokenSize)
{
    char **token = NULL;
    int currentSize = 0;
    //printf("jack tokenizer file input: %s\n", inputName);
    Constructor(inputName);
    if(!strlen(inputStream))
    {
        return token;
    }
    currentLine = strtok(inputStream, "\n\r\t\f\v");
    token = (char**)malloc(MAX_HACK_SIZE * sizeof(char*));
    if(token == NULL)
    {
        fprintf(stderr, "(JackTokenizer): Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    if(strlen(currentLine) > MAX_HACK_SIZE - 1)
    {
        fprintf(stderr, "(JackTokenizer): line length exceeded\n");
        exit(EXIT_FAILURE);
    }
    while(hasMoreTokens())
    {
        //printf("LINE: %s\n", currentLine);
        int i = 0;
        while(i < strlen(currentLine))
        {
            if(isspace(currentLine[i]))
            {
                i++;
            }
            if(isdigit(currentLine[i]))
            {
                getIntegerConstant(&i);
                //printf("INTEGER: %s\n", currentToken);
                token[currentSize] = tokenAlloc(token, currentSize, strlen(currentToken));
                strcpy(token[currentSize], currentToken);
                currentSize++;
                clearCurrentToken();
            }
            if(currentLine[i] == '"')
            {
                getStringConstant(&i);
                //printf("STRING: %s\n", currentToken);
                token[currentSize] = tokenAlloc(token, currentSize, strlen(currentToken));
                strcpy(token[currentSize], currentToken);
                currentSize++;
                clearCurrentToken();
            }
            if(strchr(symbolList, currentLine[i]) != NULL)
            {
                getSymbol(&i);
                if(currentToken[0] != '\0')
                {
                    //printf("SYMBOL: %s\n", currentToken);
                    token[currentSize] = tokenAlloc(token, currentSize, strlen(currentToken));
                    strcpy(token[currentSize], currentToken);
                    currentSize++;
                }
                clearCurrentToken();
            }
            if(isalpha(currentLine[i]) || currentLine[i] == '_')
            {
                getKeywordOrIdentifier(&i);
                //printf("KEYIDENT: %s\n", currentToken);
                token[currentSize] = tokenAlloc(token, currentSize, strlen(currentToken));
                strcpy(token[currentSize], currentToken);
                currentSize++;
                clearCurrentToken();
            }
            i++;
        }
        advance();
    }

    currentLine = NULL;
    *tokenSize = currentSize;
    free(inputStream);
    return token;
}

//CompilationEngine:

//variables to keep in mind: outputFile (not global), token (not global), currentCompileTokenIndex (global), indentLevel (global)

void printToken(FILE* outputFile, char **token)
{
    token_type tt = tokenType(token[currentCompileTokenIndex]);
    char *str_const = NULL;
    switch (tt) 
    {  
        case KEYWORD: 
            fprintf(outputFile, "<keyword> ");
            key_type key = keyWord(token[currentCompileTokenIndex]);
            switch(key)
            {
                case CLASS:       fprintf(outputFile, "class"); break;
                case METHOD:      fprintf(outputFile, "method"); break;
                case FUNCTION:    fprintf(outputFile, "function"); break;
                case CONSTRUCTOR: fprintf(outputFile, "constructor"); break;
                case INT:         fprintf(outputFile, "int"); break;
                case BOOLEAN:     fprintf(outputFile, "boolean"); break;
                case CHAR:        fprintf(outputFile, "char"); break;
                case VOID:        fprintf(outputFile, "void"); break;
                case VAR:         fprintf(outputFile, "var"); break;
                case STATIC:      fprintf(outputFile, "static"); break;
                case FIELD:       fprintf(outputFile, "field"); break;
                case LET:         fprintf(outputFile, "let"); break;
                case DO:          fprintf(outputFile, "do"); break;
                case IF:          fprintf(outputFile, "if"); break;
                case ELSE:        fprintf(outputFile, "else"); break;
                case WHILE:       fprintf(outputFile, "while"); break;
                case RETURN:      fprintf(outputFile, "return"); break;
                case TRUE:        fprintf(outputFile, "true"); break;
                case FALSE:       fprintf(outputFile, "false"); break;
                case NULL_KEY:    fprintf(outputFile, "null"); break;
                case THIS:        fprintf(outputFile, "this"); break;
                default:          fprintf(outputFile, "unknown"); break;
            }
            fprintf(outputFile, " </keyword>\n");
            break;
        case SYMBOL: 
            if(symbol(token[currentCompileTokenIndex]) == '<')
                fprintf(outputFile, "<symbol> &lt; </symbol>\n");
            else if(symbol(token[currentCompileTokenIndex]) == '>')
                fprintf(outputFile, "<symbol> &gt; </symbol>\n");
            else if(symbol(token[currentCompileTokenIndex]) == '"')
                fprintf(outputFile, "<symbol> &quot; </symbol>\n");
            else if(symbol(token[currentCompileTokenIndex]) == '&')
                fprintf(outputFile, "<symbol> &amp; </symbol>\n");
            else
                fprintf(outputFile, "<symbol> %c </symbol>\n", symbol(token[currentCompileTokenIndex]));
            break;
        case IDENTIFIER: 
            fprintf(outputFile, "<identifier> %s </identifier>\n", identifier(token[currentCompileTokenIndex]));
            break;
        case INT_CONST: 
            fprintf(outputFile, "<integerConstant> %d </integerConstant>\n", intVal(token[currentCompileTokenIndex]));
            break;
        case STRING_CONST: 
            str_const = stringVal(token[currentCompileTokenIndex]);
            fprintf(outputFile, "<stringConstant> %s </stringConstant>\n", str_const);
            free(str_const);
            break;
    }
}

void printIndent(FILE* outputFile)
{
    for(int i = 0; i < indentLevel * 2; i++)
    {
        fprintf(outputFile, " ");
    }
}

bool isClassVarDec(char *token)
{
    if(strcmp(token, "static") == 0 || strcmp(token, "field") == 0)
        return true;
    return false;
}

bool isSubroutineDec(char* token)
{
    if(strcmp(token, "constructor") == 0 || strcmp(token, "function") == 0 || strcmp(token, "method") == 0)
        return true;
    return false;
}

bool isOp(char* token)
{
    if(strcmp(token, "+") == 0 ||
        strcmp(token, "-") == 0 ||
        strcmp(token, "*") == 0 ||
        strcmp(token, "/") == 0 ||
        strcmp(token, "&") == 0 ||
        strcmp(token, "|") == 0 ||
        strcmp(token, "<") == 0 ||
        strcmp(token, ">") == 0 ||
        strcmp(token, "=") == 0)
        return true;
    return false;
}

void CompileClassVarDec(FILE* outputFile, char **token)
{
    //classVarDec: ('static' | 'field') type varName (',' varName)* ';'
    printIndent(outputFile);
    fprintf(outputFile, "<classVarDec>\n");
    indentLevel++;

    //('static' | 'field')
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //type
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //varName
    //SYMBOL TABLE
    char* kind = ToUpper(token[currentCompileTokenIndex - 2]);
    char* type = token[currentCompileTokenIndex - 1];
    char* name = token[currentCompileTokenIndex];
    if(strcmp(kind, "STATIC") == 0)
        Define(name, type, STATIC_SYMBOL);
    else if(strcmp(kind, "FIELD") == 0)
        Define(name, type, FIELD_SYMBOL);
                    //printf(">>>>>>>>>>>>>>>>>>>>>>> KIND: %s, TYPE: %s\n", kind, type);
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //(',' varName)*
    while(strcmp(token[currentCompileTokenIndex], ",") == 0)
    {
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //SYMBOL TABLE
        if(strcmp(kind, "STATIC") == 0)
            Define(token[currentCompileTokenIndex], type, STATIC_SYMBOL);
        else if(strcmp(kind, "FIELD") == 0)
            Define(token[currentCompileTokenIndex], type, FIELD_SYMBOL);
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    //';'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</classVarDec>\n");
}

void compileParameterList(FILE* outputFile, char **token)
{   
    //((type varName) (',' type varName)*)?
    //type
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //varName
    //SYMBOL TABLE
    Define(token[currentCompileTokenIndex], token[currentCompileTokenIndex - 1], ARG_SYMBOL);
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    while(strcmp(token[currentCompileTokenIndex], ",") == 0)
    {
        //','
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //type
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //varName
        //SYMBOL TABLE
        Define(token[currentCompileTokenIndex], token[currentCompileTokenIndex - 1], ARG_SYMBOL);
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
}

void compileVarDec(FILE* outputFile, char **token)
{
    //'var' type varName (',' varName)* ';'
    printIndent(outputFile);
    fprintf(outputFile, "<varDec>\n");
    indentLevel++;

    //'var'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //type
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //varName
    //SYMBOL TABLE
    char* type = token[currentCompileTokenIndex - 1];
    char* name = token[currentCompileTokenIndex];
    Define(name, type, VAR_SYMBOL);
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //(',' varName)*
    while(strcmp(token[currentCompileTokenIndex], ",") == 0)
    {
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //SYMBOL TABLE
        Define(token[currentCompileTokenIndex], type, VAR_SYMBOL);
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    //';'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</varDec>\n");
}

void CompileExpression(FILE* outputFile, char** token);

void CompileExpressionList(FILE* outputFile, char** token)
{
    //expressionList: (expression (',' expression)*)?
    printIndent(outputFile);
    fprintf(outputFile, "<expressionList>\n");
    indentLevel++;

    if(strcmp(token[currentCompileTokenIndex], ")") != 0)
    {
        //expression
        CompileExpression(outputFile, token);
        while(strcmp(token[currentCompileTokenIndex], ",") == 0)
        {
            //','
            printIndent(outputFile);
            printToken(outputFile, token);
            currentCompileTokenIndex++;
            //expression
            CompileExpression(outputFile, token);
        }
    }

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</expressionList>\n");
}

void CompileTerm(FILE* outputFile, char** token)
{
    //term: integerConstant | stringConstant | keywordConstant | varName | varName '[' expression ']' | subroutineCall | '(' expression ')' | unaryOp term
    printIndent(outputFile);
    fprintf(outputFile, "<term>\n");
    indentLevel++;

    if(strcmp(token[currentCompileTokenIndex], "-") == 0 || strcmp(token[currentCompileTokenIndex], "~") == 0)
    {
        //unaryOp
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //term
        CompileTerm(outputFile, token);
    }
    else if(strcmp(token[currentCompileTokenIndex], "(") == 0)
    {
        //'('
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //expression
        CompileExpression(outputFile, token);
        //')'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    else
    {
        //integerConstant | stringConstant | keywordConstant | varName | subroutineName | className
        printIndent(outputFile);
        /*if (tokenType(token[currentCompileTokenIndex]) == IDENTIFIER) {
            Symbol* sym = lookup(token[currentCompileTokenIndex]);
            if(sym) {
                printf("(IN EXPRESSION) Name: %s, Type: %s, Kind: %d, Index: %d\n",
                    sym->name, sym->type, sym->kind, sym->index);
            } else {
                printf("(IN EXPRESSION) Name: %s (not found)\n",
                    token[currentCompileTokenIndex]);
            }
        }*/
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        if(strcmp(token[currentCompileTokenIndex], "[") == 0) //varName '[' expression ']'
        {
            //'['
            printIndent(outputFile);
            printToken(outputFile, token);
            currentCompileTokenIndex++;
            //expression
            CompileExpression(outputFile, token);
            //']'
            printIndent(outputFile);
            printToken(outputFile, token);
            currentCompileTokenIndex++;
        }
        else if(strcmp(token[currentCompileTokenIndex], "(") == 0 || strcmp(token[currentCompileTokenIndex], ".") == 0) //subroutineCall: subroutineName '(' expressionList ')' | (className | varName) '.' subroutineName '(' expressionList ')'
        {
            if(strcmp(token[currentCompileTokenIndex], "(") == 0)
            {
                //'('
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
                //expressionList
                CompileExpressionList(outputFile, token);
                //')'
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
            }
            else if(strcmp(token[currentCompileTokenIndex], ".") == 0)
            {
                //'.'
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
                //subroutineName
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
                //'('
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
                //expressionList
                CompileExpressionList(outputFile, token);
                //')'
                printIndent(outputFile);
                printToken(outputFile, token);
                currentCompileTokenIndex++;
            }
        }
    }

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</term>\n");
    //printf("COMPILE TERM DONE\n");
}

void CompileExpression(FILE* outputFile, char** token)
{
    //expression: term (op term)*
    printIndent(outputFile);
    fprintf(outputFile, "<expression>\n");
    indentLevel++;

    CompileTerm(outputFile, token);
    while(isOp(token[currentCompileTokenIndex]))
        {
            printIndent(outputFile);
            printToken(outputFile, token);
            currentCompileTokenIndex++;
            CompileTerm(outputFile, token);
        }

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</expression>\n");
}

void compileStatements(FILE* outputFile, char **token);

void compileDo(FILE* outputFile, char** token)
{
    //doStatement: 'do' subroutineCall ';'
    printIndent(outputFile);
    fprintf(outputFile, "<doStatement>\n");
    indentLevel++;

    //'do'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //subroutineCall: subroutineName '(' expressionList ')' | (className | varName) '.' subroutineName '(' expressionList ')'
    //subroutineName | className | varName
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    if(strcmp(token[currentCompileTokenIndex], "(") == 0)
    {
        //'('
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //expressionList
        CompileExpressionList(outputFile, token);
        //')'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    else if(strcmp(token[currentCompileTokenIndex], ".") == 0)
    {
        //'.'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //subroutineName
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //'('
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //expressionList
        CompileExpressionList(outputFile, token);
        //')'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    //';'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;


    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</doStatement>\n");
}

void compileLet(FILE* outputFile, char** token)
{
    //letStatement: 'let' varName ('[' expression ']')? '=' expression ';'
    printIndent(outputFile);
    fprintf(outputFile, "<letStatement>\n");
    indentLevel++;

    //'let'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //varName
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //('[' expression ']')?
    if(strcmp(token[currentCompileTokenIndex], "[") == 0)
    {
        //'['
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //expression
        CompileExpression(outputFile, token);
        //']'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }
    //'='
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //expression
    CompileExpression(outputFile, token);
    //';'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</letStatement>\n");
}

void compileWhile(FILE* outputFile, char** token)
{
    //whileStatement: 'while' '(' expression ')' '{' statements '}'
    printIndent(outputFile);
    fprintf(outputFile, "<whileStatement>\n");
    indentLevel++;

    //'while'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //'('
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //expression
    CompileExpression(outputFile, token);
    //')'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //'{'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //statements
    compileStatements(outputFile, token);
    //'}'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</whileStatement>\n");
}

void compileReturn(FILE* outputFile, char** token)
{
    //'return' expression? ';'
    printIndent(outputFile);
    fprintf(outputFile, "<returnStatement>\n");
    indentLevel++;

    //'return'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //expression?
    if(strcmp(token[currentCompileTokenIndex], ";") != 0)
    {
        CompileExpression(outputFile, token);
    }
    //';'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</returnStatement>\n");
}

void compileIf(FILE* outputFile, char** token)
{
    //'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
    printIndent(outputFile);
    fprintf(outputFile, "<ifStatement>\n");
    indentLevel++;

    //'if'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //'('
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //expression
    CompileExpression(outputFile, token);
    //')'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //'{'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //statements
    compileStatements(outputFile, token);
    //'}'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //('else' '{' statements '}')?
    if(strcmp(token[currentCompileTokenIndex], "else") == 0)
    {
        //'else'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //'{'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
        //statements
        compileStatements(outputFile, token);
        //'}'
        printIndent(outputFile);
        printToken(outputFile, token);
        currentCompileTokenIndex++;
    }

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</ifStatement>\n");
}

void compileStatements(FILE* outputFile, char **token)
{
    //statements: statement*
    printIndent(outputFile);
    fprintf(outputFile, "<statements>\n");
    indentLevel++;

    //statement: letStatement | ifStatement | whileStatement | doStatement | returnStatement
    while(strcmp(token[currentCompileTokenIndex], "let") == 0 ||
        strcmp(token[currentCompileTokenIndex], "if") == 0 ||
        strcmp(token[currentCompileTokenIndex], "while") == 0 ||
        strcmp(token[currentCompileTokenIndex], "do") == 0 ||
        strcmp(token[currentCompileTokenIndex], "return") == 0)
        {
            if(strcmp(token[currentCompileTokenIndex], "let") == 0)
            {
                compileLet(outputFile, token);
            }
            if(strcmp(token[currentCompileTokenIndex], "if") == 0)
            {
                compileIf(outputFile, token);
            }
            if(strcmp(token[currentCompileTokenIndex], "while") == 0)
            {
                compileWhile(outputFile, token);
            }
            if(strcmp(token[currentCompileTokenIndex], "do") == 0)
            {
                compileDo(outputFile, token);
            }
            if(strcmp(token[currentCompileTokenIndex], "return") == 0)
            {
                compileReturn(outputFile, token);
            }
        }

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</statements>\n");
}

void CompileSubroutine(FILE* outputFile, char **token)
{
    startSubroutine();

    //subroutineDec: ('constructor' | 'function' | 'method') ('void' | type) subroutineName '(' parameterList ')' subroutineBody
    printIndent(outputFile);
    fprintf(outputFile, "<subroutineDec>\n");
    indentLevel++;

    //('constructor' | 'function' | 'method')
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //('void' | type)
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //subroutineName
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //'('
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //parameterList
    printIndent(outputFile);
    fprintf(outputFile, "<parameterList>\n");
    indentLevel++;
    if(strcmp(token[currentCompileTokenIndex], ")") != 0)
    {
        compileParameterList(outputFile, token);
    }
    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</parameterList>\n");
    //')'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;

    //subroutineBody
    //'{' varDec* statements '}'
    printIndent(outputFile);
    fprintf(outputFile, "<subroutineBody>\n");
    indentLevel++;
    //'{'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //varDec*
    while(strcmp(token[currentCompileTokenIndex], "var") == 0)
        compileVarDec(outputFile, token);
    //statements
    compileStatements(outputFile, token);
    //'}'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</subroutineBody>\n");

    indentLevel--;
    printIndent(outputFile);
    fprintf(outputFile, "</subroutineDec>\n");
}

void CompileClass(FILE* outputFile, char **token)
{
    SymbolTableConstructor();

    //class: 'class' className '{' classVarDec* subroutineDec* '}'
    indentLevel++;
    fprintf(outputFile, "<class>\n");
    //'class'
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //className
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //{
    printIndent(outputFile);
    printToken(outputFile, token);
    currentCompileTokenIndex++;
    //classVarDec*
    while(isClassVarDec(token[currentCompileTokenIndex]))
        CompileClassVarDec(outputFile, token);
    //subroutineDec*
    while(isSubroutineDec(token[currentCompileTokenIndex]))
        CompileSubroutine(outputFile, token);
    //}
    printIndent(outputFile);
    printToken(outputFile, token);

    indentLevel--;
    fprintf(outputFile, "</class>\n");
}

void CompilationEngine(FILE* outputFile, char** token) //Constructor
{
    CompileClass(outputFile, token);
}

//JackAnalyzer:

int inputType(const char *inputName) //1 - directory, 0 - file
{
    if(strstr(inputName, ".jack") != NULL)
        return 0;
    return 1;
}

void analyzerLogic(char *inputName, char *fileName) //if input is file, fileName is NULL, since the inputName is the fileName already and not a directory
{
    char **token;
    int tokenSize = 0;
    if(inputType(inputName) == 0)
    {
        token = JackTokenizer(inputName, &tokenSize); //Create a JackTokenizer from the Xxx.jack file
        printf("tokenized: %s\n", inputName);
    }
    else if(inputType(inputName) == 1)
    {
        char inputFileName[4097] = "";
        strcat(inputFileName, "./");
        strcat(inputFileName, inputName);
        strcat(inputFileName, "/");
        strcat(inputFileName, fileName);
        token = JackTokenizer(inputFileName, &tokenSize);
        printf("tokenized: %s\n", fileName);
    }

    FILE* outputFile = NULL; //the output .xml file
    FILE* outputTokenizerFile = NULL; //the output tokenizer .xml file (For each source file Xxx.jack, have your tokenizer give the output file the name XxxT.xml)
    FILE* outputVMFile = NULL; //the output .vm file
    if(inputType(inputName) == 0) //Create an output file called Xxx.xml and prepare it for writing in the current directory
    {
        char outputName[256] = "";
        char outputTokenizerName[256] = "";
        char outputVMName[256] = "";
        for(int i = 0; i < strlen(inputName); i++)
        {
            outputName[i] = inputName[i];
            if(inputName[i] == '.')
                break;
        }
        outputName[strlen(outputName) - 1] = '\0'; 

        strcpy(outputVMName, outputName);
        strcpy(outputTokenizerName, outputName);
        strcat(outputTokenizerName, "T.xml");

        strcat(outputName, ".xml");
        outputFile = fopen(outputName, "w");
        if(outputFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output file\n");
            exit(EXIT_FAILURE);
        }
        printf("created output file: %s\n", outputName);

        outputTokenizerFile = fopen(outputTokenizerName, "w");
        if(outputTokenizerFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output tokenizer file\n");
            exit(EXIT_FAILURE);
        }
        printf("created output tokenizer file: %s\n", outputTokenizerName);

        strcat(outputVMName, ".vm");
        outputVMFile = fopen(outputVMName, "w");
        if(outputVMFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output VM file\n");
            exit(EXIT_FAILURE);
        }
        printf("created output VM file: %s\n", outputVMName);

    }
    else if(inputType(inputName) == 1) //Create an output file called Xxx.xml and prepare it for writing in the directory given by input
    {
        char outputName[256] = "";
        char outputTokenizerName[256] = "";
        char outputVMName[256] = "";
        for(int i = 0; i < strlen(fileName); i++)
        {
            outputName[i] = fileName[i];
            if(fileName[i] == '.')
                break;
        }
        outputName[strlen(outputName) - 1] = '\0'; 

        strcpy(outputTokenizerName, outputName);
        strcat(outputTokenizerName, "T.xml");

        strcpy(outputVMName, outputName);
        strcat(outputVMName, ".vm");

        strcat(outputName, ".xml");

        char outputPath[4097] = "";
        strcat(outputPath, "./");
        strcat(outputPath, inputName);
        strcat(outputPath, "/");
        strcat(outputPath, outputName);

        char outputPathTokenizer[4097] = "";
        strcat(outputPathTokenizer, "./");
        strcat(outputPathTokenizer, inputName);
        strcat(outputPathTokenizer, "/");
        strcat(outputPathTokenizer, outputTokenizerName);

        char outputPathVM[4097] = "";
        strcat(outputPathVM, "./");
        strcat(outputPathVM, inputName);
        strcat(outputPathVM, "/");
        strcat(outputPathVM, outputVMName);

        outputFile = fopen(outputPath, "w");
        if(outputFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output file\n");
            exit(EXIT_FAILURE);
        }
        //printf("current output path: %s\n", outputPath);
        printf("created output file: %s\n", outputName);

        outputTokenizerFile = fopen(outputPathTokenizer, "w");
        if(outputTokenizerFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output tokenizer file\n");
            exit(EXIT_FAILURE);
        }
        printf("created output tokenizer file: %s\n", outputTokenizerName);

        outputVMFile = fopen(outputPathVM, "w");
        if(outputVMFile == NULL)
        {
            fprintf(stderr, "(analyzerLogic): error opening output VM file\n");
            exit(EXIT_FAILURE);
        }
        printf("created output VM file: %s\n", outputVMName);
    }
    printf("token size: %d\n", tokenSize);

    //tokenizer output:
    fprintf(outputTokenizerFile, "<tokens>\n");
    for(int i = 0; i < tokenSize; i++)
    {
        token_type tt = tokenType(token[i]);
        char *str_const = NULL;
        switch (tt) 
        {  
            case KEYWORD: 
                fprintf(outputTokenizerFile, "<keyword> ");
                key_type key = keyWord(token[i]);
                switch(key)
                {
                    case CLASS:       fprintf(outputTokenizerFile, "class"); break;
                    case METHOD:      fprintf(outputTokenizerFile, "method"); break;
                    case FUNCTION:    fprintf(outputTokenizerFile, "function"); break;
                    case CONSTRUCTOR: fprintf(outputTokenizerFile, "constructor"); break;
                    case INT:         fprintf(outputTokenizerFile, "int"); break;
                    case BOOLEAN:     fprintf(outputTokenizerFile, "boolean"); break;
                    case CHAR:        fprintf(outputTokenizerFile, "char"); break;
                    case VOID:        fprintf(outputTokenizerFile, "void"); break;
                    case VAR:         fprintf(outputTokenizerFile, "var"); break;
                    case STATIC:      fprintf(outputTokenizerFile, "static"); break;
                    case FIELD:       fprintf(outputTokenizerFile, "field"); break;
                    case LET:         fprintf(outputTokenizerFile, "let"); break;
                    case DO:          fprintf(outputTokenizerFile, "do"); break;
                    case IF:          fprintf(outputTokenizerFile, "if"); break;
                    case ELSE:        fprintf(outputTokenizerFile, "else"); break;
                    case WHILE:       fprintf(outputTokenizerFile, "while"); break;
                    case RETURN:      fprintf(outputTokenizerFile, "return"); break;
                    case TRUE:        fprintf(outputTokenizerFile, "true"); break;
                    case FALSE:       fprintf(outputTokenizerFile, "false"); break;
                    case NULL_KEY:    fprintf(outputTokenizerFile, "null"); break;
                    case THIS:        fprintf(outputTokenizerFile, "this"); break;
                    default:          fprintf(outputTokenizerFile, "unknown"); break;
                }
                fprintf(outputTokenizerFile, " </keyword>\n");
                break;
            case SYMBOL: 
                if(symbol(token[i]) == '<')
                    fprintf(outputTokenizerFile, "<symbol> &lt; </symbol>\n");
                else if(symbol(token[i]) == '>')
                    fprintf(outputTokenizerFile, "<symbol> &gt; </symbol>\n");
                else if(symbol(token[i]) == '"')
                    fprintf(outputTokenizerFile, "<symbol> &quot; </symbol>\n");
                else if(symbol(token[i]) == '&')
                    fprintf(outputTokenizerFile, "<symbol> &amp; </symbol>\n");
                else
                    fprintf(outputTokenizerFile, "<symbol> %c </symbol>\n", symbol(token[i]));
                break;
            case IDENTIFIER: 
                fprintf(outputTokenizerFile, "<identifier> %s </identifier>\n", identifier(token[i]));
                break;
            case INT_CONST: 
                fprintf(outputTokenizerFile, "<integerConstant> %d </integerConstant>\n", intVal(token[i]));
                break;
            case STRING_CONST: 
                str_const = stringVal(token[i]);
                fprintf(outputTokenizerFile, "<stringConstant> %s </stringConstant>\n", str_const);
                free(str_const);
                break;
        }
    }
    fprintf(outputTokenizerFile, "</tokens>\n");

    currentCompileTokenIndex = 0;
    CompilationEngine(outputFile, token); //Use the CompilationEngine to compile the input JackTokenizer into the output file

    if(token != NULL)
    {
        for(int i = 0; i < tokenSize; i++)
        {
            if(token[i] != NULL)
                free(token[i]);
        }
        if(token != NULL)
            free(token);
    }

    fclose(outputFile);
    fclose(outputTokenizerFile);
}

void JackAnalyzer(char *inputName)
{
    if(inputType(inputName))
    {
        printf("argument is: directory\n");
        char pathName[4097] = "./";
        strcat(pathName, inputName);
        DIR* dir = opendir(pathName);
        if(dir == NULL)
        {
            fprintf(stderr, "(JackAnalyzer): error opening directory\n");
            exit(EXIT_FAILURE);
        }
        struct dirent* entity;
        entity = readdir(dir);
        while(entity != NULL)
        {
            //printf("%s\n", entity->d_name);
            if(strstr(entity->d_name, ".jack") != NULL)
            {
                printf(".jack file found: %s\n", entity->d_name);
                analyzerLogic(inputName, entity->d_name);
            }
            entity = readdir(dir);
        }
        closedir(dir);
    }
    else
    {
        printf("argument is: file\n");
        analyzerLogic(inputName, NULL);
    }
}

//main:
int main(int argc, char** argv)
{
    printf("%s\n", "~~~ Luca's Jack Analyzer ~~~");

    if(argc <= 1)
    {
        fprintf(stderr, "(main): not enough input arguments");
        exit(EXIT_FAILURE);
    }
    JackAnalyzer(argv[1]);

    return 0;
}