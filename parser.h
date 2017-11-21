#ifndef PARSER_H
#define PARSER_H

#include "symbol.h"
#include "types.h"
#include "dependency.h"

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
extern int integral_token_val;   // value of current token (mainly for number)
extern double real_token_val;   // value of current token (mainly for number)
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

extern int parser_init();

static int type_of_token(int token);

static void load_real_number_constant(double float_const);

static void load_integral_number_constant(int int_const);

static int get_base_type(int type);

static int emit_store_directive(int type);

static int emit_load_directive(int type);

static void check_assignment_types(int left_type, int right_type);

static Boolean does_operate_on_constant();

static void emit_code_for_binary_left ( int** reserve1, int** reserve2);

static void emit_code_for_binary_right
(
   int operator_for_real_number,
   int operator_for_integral_number,
   int** reserve1,
   int** reserve2
);

static void numtype_to_strtype(int num_type, char* repr);

static int* relocation();

static  void init_symbol_table();

extern int* dependency_inject
(
    struct dependency_items* dep_itemsp,   
    const char* src_code
);

static int* code_start;

#endif 

