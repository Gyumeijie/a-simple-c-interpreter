#include "executor.h" 
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

//虚拟机寄存器
static int *bp, ax, cycle; 
static int* stack;


int interpreter_init()
{
    //运行是会需要，该部分只要虚拟机运行就行了
    if (!(stack = malloc(STACK_SIZE))) {
        printf("could not malloc(%d) for stack area\n", STACK_SIZE);
        return -1;
    }

}


void run_code(int* code_start)
{
   //初始化堆栈
   int* sp = (int *)((int)stack + STACK_SIZE);
   eval(code_start, sp);
}


static int eval(int* pc, int* sp) {
    int op, *args;
    cycle = 0;
    //临时增加用来保存浮点数的
    double bx;
    while (1) {
        cycle ++;
        //TODO 在有main函数的时候是从main函数开始执行的，如果要去掉main函数的化
        //就要正确设置pc否则就会内存错误
        op = *pc++; 

        //TODO 使用switch 减少无效的if/else判断，因为在调试的时候发现要查找某个
        //op的时候 如果op很后面那么前面就需要进行很多的if/else的条件判断
 
        if (1) {
            printf("%d> %.4s", cycle,
                   & "LEA ,IMM ,FIMM,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LD  ,"
                   "LF,  LI  ,LC  ,SD  ,SF  ,SI  ,SC  ,PUSH,OR  ,XOR ,AND ,"
                   "EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                   "OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT"[op * 5]);
            if (op <= ADJ)
                printf(" %0x\n", *pc);
            else
                printf("\n");
        }

        //加载立即数到寄存器ax中，加载整数以及地址
        if (op == IMM)       {ax = *pc++;}                                     

        //TODO 加载float类型的常量，这里的常量是内部的而不是外部导入的
        //外部导入的都是通过地址去加载的
        else if (op == FIMM)       {bx = *pc++; printf("bx is %f\n", bx);}                                     

        //加载字符类型数据到ax中,原来ax中保存的是地址
        else if (op == LC)   {ax = *(char *)ax;}                              

        //加载整型数据到ax中,原来ax中保存的是地址
        else if (op == LI)   {ax = *(int *)ax;}        

        else if (op == LF)   {bx = *(float *)ax;}        
        
        else if (op == LD)   {bx = *(double *)ax;}        

        else if (op == SC)   {ax = *(char *)*sp++ = ax;} 
        else if (op == SI)   {*(int *)*sp++ = ax;}        

        // TODO 因为外部注入的变量类型可能是float类型的也可能是double类型的
        // 所以存储的时候需要区分开来因此设计了两条指令
        //存储float类型的
        else if (op == SF)   {*(float*)*sp++ = bx;}      
        //存储double类型
        else if (op == SD)   {*(double*)*sp++ = bx;}      

        else if (op == PUSH) {*--sp = ax;}                                    
        else if (op == JMP)  {pc = (int *)*pc;}                              
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}               
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}      
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;} 
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}     
        else if (op == ADJ)  {sp = sp + *pc++;}                              
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;} 
        else if (op == LEA)  {ax = (int)(bp + *pc++);}  

        //逻辑运算符
        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;

        //提供必要的一些公共函数
        //只要根据相应的op代码执行特定的动作就行了
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp);}
        //是不是printf只能处理6个参数
        else if (op == PRTF) { args = sp + pc[1]; ax = printf((char *)args[-1], args[-2], args[-3], args[-4], args[-5], args[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
    
        //下面的提供私有的函数例程，为了不会因为增加专用的函数例程导致主执行器
        //的switch变得太大，就将这些私有的移到专门的执行器中

        //1xxx xxxx 私有的代码表示方法 
        // op = PRIV_OP - 128 再调用具体的一个执行器 
        // private_executor(text, data, op){
        //     
        //
        //}
        
        //唯一退出的代码
        else if (op == EXIT) { printf("exit(%d)\n", *sp); return *sp;}
        else {
            printf("unknown instruction:%d\n", op);
            return -1;
        }
    }
}




