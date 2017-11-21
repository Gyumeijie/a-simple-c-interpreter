#ifndef PARSER_H
#define PARSER_H

#include "symbol.h"
#include "types.h"
#include "dependency.h"

enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

static int  *text_start;
static char *data_start;

static int *text, // text segment
           *stack;// stack

static char *src;

static int poolsize; 

char *data; 
extern int *current_id; 
extern int line;       
extern int integral_token_val;  
extern double real_token_val;   
extern int token; 
extern int num_type;
static int  *symbols; 
static int basetype;  
static int expr_type;


static void expression(int level); 

static void statement();

static void parse_block_code();

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

static void reset_complie_environment();

static  void init_symbol_table();

extern int* compile_src_code
(
    struct dependency_items* dep_itemsp,   
    const char* src_code
);

static void  inject_dependency(struct dependency_items* dep_itemsp);

#endif 

