#ifndef PARSER_H
#define PARSER_H

#include "symbol.h"

static int debug;    // print the executed instructions
static int assembly; // print out the assembly and source

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};


// types of variable/function
// 其中DOUBLE使用来区分外部变量是float类型还是double类型的浮点数
// 内部的浮点数只有float类型
enum { CHAR, INT, FLOAT, DOUBLE, PTR };

// type of declaration.
enum {Global, Local};

static int actual_text_len, *text_start;
static int actual_data_len; 
static char *data_start;

static int *text, // text segment
           *stack;// stack
static int * old_text; // for dump text segment
static int *idmain;

static char *src, *old_src;  // pointer to source code string;

static int poolsize; // default size of text/data/stack
static int *pc, *bp, *sp, ax, cycle; // virtual machine registers

char *data; // data segment
extern int *current_id; // current parsed ID
extern int line;       // line number of source code
extern int token_val;   // value of current token (mainly for number)
extern int token; // current token
//TODO 进一步区分数值类型
extern int num_type;

static int  *symbols;   // symbol table

static int basetype;    // the type of a declaration, make it global for convenience
static int expr_type;   // the type of an expression

// function frame
//
// 0: arg 1
// 1: arg 2
// 2: arg 3
// 3: return address
// 4: old bp pointer  <- index_of_bp
// 5: local var 1
// 6: local var 2
static int index_of_bp; // index of bp pointer on stack

//TODO 新增
static int* code_start;

static void expression(int level); 

static void statement();

static void parse_configuration();

extern int init();

static int* relocation();

static  void init_symbol_table();

extern int* dependency_inject
(
    char* sym, 
    void *extern_addr,
    const char* src_code
);

static int* code_start;

#endif 

