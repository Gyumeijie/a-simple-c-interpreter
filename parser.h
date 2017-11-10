#ifndef PARSER_H
#define PARSER_H

#include "symbol.h"

static int debug;    // print the executed instructions
static int assembly; // print out the assembly and source

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};


// types of variable/function
enum { CHAR, INT, PTR };

// type of declaration.
enum {Global, Local};

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

//TODO ÐÂÔö
static int* code_start;

static void expression(int level); 

static void statement();

static void parse_configuration();

extern int init();

static  void init_symbol_table();

extern int* dependency_inject
(
    char* sym, 
    int *extern_addr,
    const char* src_code
);

static int* code_start;

#endif 

