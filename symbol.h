#ifndef SYMBOL_H
#define SYMBOL_H

// 指令 
enum{ 
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL,DIV, 
    // TODO 将公共函数部分分割开来
    MOD, OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};


// 标记 
enum {
   Num = 128, Fun, Sys, Glo, Ext, Id,
   Char, Else, Enum, If, Int, Return, Sizeof, While,
   //TODO 将操作符单独分出去
   Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt,
   Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// 重定位
enum {Text_Rel, Data_Rel};
#endif
