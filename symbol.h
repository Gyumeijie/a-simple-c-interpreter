#ifndef SYMBOL_H
#define SYMBOL_H

// 指令 
enum{ 
    LEA, IMM, FIMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LD, LF, LI, LC, SD, SF, 
    SI, SC, PUSH, OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, 
    MUL,DIV, 
    // TODO 将公共函数部分分割开来
    MOD, OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};


// 标记 
enum {
   Num = 128, Fun, Sys, Glo, Ext, Id,
   //Char, Else, Enum, If, Int, Return, Sizeof, While,
   Char, Int, Float, If, Else, While, Return

   //TODO 弄清楚为什么删掉前面的东西会影响结果
   //Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt,
   //Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

enum {
   //TODO 将操作符单独分出去
   Assign = 256, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt,
   Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};
// 重定位
enum {Text_Rel, Data_Rel};

#endif
