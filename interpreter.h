#ifndef XC_H
#define XC_H

static int debug;    // print the executed instructions
static int assembly; // print out the assembly and source
static int token; // current token

// instructions
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

// tokens and classes (operators last and in precedence order)
// copied from c4
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};


// types of variable/function
enum { CHAR, INT, PTR };

// type of declaration.
enum {Global, Local};

static int *text, // text segment
           *stack;// stack
static int * old_text; // for dump text segment
static char *data; // data segment
static int *idmain;

static char *src, *old_src;  // pointer to source code string;

static int poolsize; // default size of text/data/stack
static int *pc, *bp, *sp, ax, cycle; // virtual machine registers

static int *current_id, // current parsed ID
    *symbols,    // symbol table
    line,        // line number of source code
    token_val;   // value of current token (mainly for number)

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

static void next();
static void match(int tk);

static void expression(int level); 

static void statement();

static void function_parameter();

static void function_body();

static void function_declaration(); 

static void global_declaration();

static void program();

static void parse_configuration();

static int eval(); 

extern int init();

static  void init_symbol_table();

extern int* dependency_inject
(
    char* sym, 
    int *extern_addr,
    const char* src_code
);

extern void run_code(int* code_start);

static int* code_start;
#endif 

