#ifndef SYMBOL_H
#define SYMBOL_H

// 指令 
enum{ 
    LEA, IMM, FIMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LD, LF, LI, LC, SD, SF, 
    SI, SC, ATOB, BTOA, PUSF, PUSH, OR, XOR, AND, EQF, EQ, NEF, NE, LTF, LT,
    GTF, GT, LEF, LE, GEF, GE, SHL, SHR, ADDF, ADD, SUB,MULF ,MUL, DIVF, DIV, 
    MOD, NOP,
    
    //公共函数也作为指令
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};


// 标记 
enum {
   Num = 128, Fun, Sys, Glo, Ext, Id,
   Char, Int, Float, Double, If, Else, While, Return

};

enum {
   Assign = 256, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt,
   Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// 重定位类型
enum {Text_Rel, Data_Rel};

// 变量类型
enum { CHAR, INT, FLOAT, DOUBLE, PTR};

#endif
