#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "parser.h"
#include "executor.h"
#include "lex.h"
#include "relocation.h"
#include "dependency.h"

/**
 *
 * 确定了表达式的类型expr_type
 */
static void expression(int level) {
    // expressions have various format.
    // but majorly can be divided into two parts: unit and operator
    // for example `(char) *a[10] = (int *) func(b > 0 ? 10 : 20);
    // `a[10]` is an unit while `*` is an operator.
    // `func(...)` in total is an unit.
    // so we should first parse those unit and unary operators
    // and then the binary ones
    //
    // also the expression can be in the following types:
    // 表达式的BNF范式
    // 1. unit_unary ::= unit | unit unary_op | unary_op unit
    // 2. expr ::= unit_unary (bin_op unit_unary ...)

    // unit_unary()
    int *id;
    int tmp;
    int *addr;

    {
        if (!token) {
            printf("%d: unexpected token EOF of expression\n", line);
            exit(-1);
        }

        // 处理数值
        if (token == Num) {
            match(Num);
            //TODO 进一步判断是否是浮点类型

            if (num_type == INT){
               load_int_constant(token_val);
               expr_type = INT;
            }else{
            //加载浮点常量
               load_float_constant(token_val_float);
               expr_type = FLOAT;
            }
        }

        // 处理字符串常量
        else if (token == '"') {

            *++text = IMM;
            *++text = token_val;

            match('"');
            while (token == '"') {
                match('"');
            }

            // 字符串常量不需要重定位
            // data段初始化的时候都为0，所以不需要显示的在末尾添加'\0'，下面是
            // 为了使得data段在4字节边界上对齐，例如如果字符串的长度为11个字节
            // 的话，那么对齐后实际分配的data空间是12个字节
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));

            expr_type = PTR;
        }

        // 处理标识符
        else if (token == Id) {

            match(Id);
            id = current_id;

            //函数调用
            if (token == '(') {
                match('(');

                int num_args = 0; //实参的个数
                while (token != ')') {
                    // 将参数压人栈中
                    expression(Assign);
                    *++text = PUSH;
                    num_args++;

                    if (token == ',') {
                        match(',');
                    }

                }
                match(')');

                // 系统函数, id[Value]保存的是函数的OP代码
                if (id[Class] == Sys) {
                    *++text = id[Value];
                }
                // 自定义的函数
                else if (id[Class] == Fun) {
                    *++text = CALL;
                    *++text = id[Value];
                }
                else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }

                // 如果函数调用有传递参数，那么函数返回后需要清理这些参数对应的
                // 栈空间
                if (num_args > 0) {
                    *++text = ADJ;
                    *++text = num_args;
                }

                //变量的类型
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
            // 枚举类型
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else {
            // 普通变量 
            
                if (id[Class] == Ext) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]都是保存其地址
                }
                else if (id[Class] == Glo) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]都是保存其地址
                    add_relocation_item(text, (id[Value] - (int)data_start), Data_Rel);                    
                }
                else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }


                expr_type = id[Type];

                //根据变量的类型选择相应的加载指令
                *++text = emit_load_directive(expr_type);
            }
        }

        // 强制类型转换以及不同的括号分组
        else if (token == '(') {
            match('(');
            // 强制类型转换
            if (token == Int || token == Char || token == Float) {
                int cast_type = type_of_token(token);
                match(token);
                while (token == Mul) {
                    match(Mul);
                    cast_type = cast_type + PTR;
                }
                match(')');

                //转型的优先级和Inc(++)一样
                expression(Inc); 

                // 强制类型转换整体的表达式的类型应该和转型的类型是一样的
                // (int **)var, 那么不管var之前是什么类型的变量，转型后的类型
                // 就是(int **)
                expr_type  = cast_type;

            } else {
            // 普通的括号分组
                expression(Assign);
                match(')');
            }
        }

        else if (token == Mul) {
            match(Mul);

            //解引用的优先级和Inc(++)一样
            expression(Inc); 

            printf("expr_type %d\n", expr_type);
            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            //float** f;   1.0 + **f
            //那么通过Load操作逐步解引用addr (LI) (LF)
            //
            *++text = emit_load_directive(expr_type); 
        }

        else if (token == And) {
            match(And);

            //取地址的优先级和Inc(++)一样
            expression(Inc); 

            //如果是&var的话，直接通过load操作前面的IMM操作就可以加载其地址了
            //"&"后面的只能是变量而不能是常量，但是这里存在一个bug: 如果&const
            //而这个const的数值恰好是LC LI LF和LD其中一个，所以为了保险起见加上
            //对这种情况的判断;其次&的优先级比较高所以像&(1+2)之类的都是不合法的
            if (!does_operate_on_constant() &&
                 (*text == LC || *text == LI || *text == LF || *text == LD)){
                text--;
            }else {
                printf("%d: bad address of\n", line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }

        else if (token == '!') {
            match('!');

            //逻辑非的优先级和Inc(++)一样
            expression(Inc);

            // 使用expr == 0 进行判断
            // 如果是"!"后面的表达式类型是浮点类型，则将bx寄存器中的数转型成整
            // 型并放置在ax中指令BTOA就是这个作用
            if (expr_type == FLOAT || expr_type == DOUBLE){
                *++text = BTOA;                
            }

            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            //最后整个表达式(!<expr>)的类型是INT
            expr_type = INT;
        }

        else if (token == '~') {
            // bitwise not
            match('~');

            //按位非的优先级和Inc(++)一样
            expression(Inc); 
        
            //位操作的话表达式的类型一定要正确，因此需要检查一些类型
            if (expr_type == FLOAT || expr_type == DOUBLE){
                printf("%d: wrong type argument to bit-complement\n", line);
                exit(-1); 
            }

            //使用<expr> XOR -1来时实现按位非，具体细节如下
            //(1111 1111)  -1
            //(0110 0011)  XOR
            //______________
            //
            //(1001 1100)
            *++text = PUSH; 
            *++text = IMM;  
            *++text = -1;
            *++text = XOR;

            //最后整个表达式(~<expr>)的类型是INT
            expr_type = INT;
        }
        else if (token == Add) {
            // +var, 不做实际的操作
            match(Add);

            //正号优先级和Inc(++)一样
            expression(Inc);

            //最后整个表达式(+<expr>)的类型和<expr>相同
            expr_type = expr_type;
        }
        else if (token == Sub) {
            // -var
            match(Sub);

            if (token == Num) {
                if (num_type == INT || num_type == CHAR){
                   load_int_constant(-token_val);
                }else{
                   load_float_constant(-token_val_float);
                }
                match(Num);
            } else {
                //TODO 
                *++text = IMM;
                *++text = -1;   
                *++text = PUSH;
                expression(Inc);
                *++text = MUL; 
            }

        }

        else if (token == Inc || token == Dec) {
            int save_token = token;
            match(token);
            expression(Inc);

            if (does_operate_on_constant()){
                printf("%d:Inc or Dec cannot apply on constant\n", line);
                exit(-1);
            } 

            // 暂时不支持浮点类型的变量(包括指针类型)++或--操作
            if (get_base_type(expr_type) > INT){
                printf("%d: sorry, Inc or Dec is not supported for floating\n",
                      line);
                exit(-1);
            }


            if (*text == LC) {
                *text = PUSH;  
                *++text = LC;
            } else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line);
                exit(-1);
            }

            *++text = PUSH;
            *++text = IMM;
            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (save_token == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }
        else {
            printf("%d: bad expression\n", line);
            exit(-1);
        }
    }
 

    //处理二元操作符以及后缀操作符
    {
        // 根据当前的操作符优先级进行操作
        while (token >= level) {
            int left_type = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);

                // 变量如果是充当左值话就修改指令，使用PUSH指令保存其地址
                // 如果是用作右值的话，就使用Load指令加载
                // 左值不是变量，报错
                if (*text == LC || *text == LI || *text == LF || *text == LD) {   
                    *text = PUSH; 
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                // 然后计算右边表达式的值，并将结果保存到ax或者bx
                expression(Assign);

                //类型兼容的函数
                printf("assign left %d , right %d\n", left_type, expr_type);
                check_assignment_types(left_type, expr_type);

                //如果两个是类型兼容的话，那么整个表达式的类型就是左操作数的类型
                expr_type = left_type; 
                *++text = emit_store_directive(expr_type);
            }

            else if (token == Cond) {
                // expr ? a : b;
                match(Cond);

                // 如果结果是float类型的，那么将bx中的数转型移到ax中
                // 转型的精度损失不会影响条件的真假性
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                int offset = (text + 3 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 3);
                *++text = JMP;

                addr = ++text;
                expression(Cond);
                offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);
            }

            else if (token == Lor) {
                // logic or
                match(Lor);

                // 如果结果是float类型的，那么将bx中的数转型移到ax中
                // 转型的精度损失不会影响条件的真假性
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JNZ;
                addr = ++text;
                expression(Lan);

                int offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Lan) {
                // logic and
                match(Lan);

                // 如果结果是float类型的，那么将bx中的数转型移到ax中
                // 转型的精度损失不会影响条件的真假性
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JZ;
                addr = ++text;
                expression(Or);

                int offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);

                expr_type = INT;
            }
            else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);

               //位操作的话表达式的类型一定要正确，因此需要检查一些类型
               if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise or\n", line);
                   exit(-1); 
                }

                *++text = OR;
                expr_type = INT;
            }
            else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);

                //位操作的话表达式的类型一定要正确，因此需要检查一些类型
                if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise xor\n", line);
                   exit(-1); 
                }

                *++text = XOR;
                expr_type = INT;
            }
            else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);

                //位操作的话表达式的类型一定要正确，因此需要检查一些类型
                if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise xor\n", line);
                   exit(-1); 
                }

                *++text = AND;
                expr_type = INT;
            }
            else if (token == Eq) {
                // equal ==
                match(Eq);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;

                expression(Ne);

                //*++text = EQ;
                emit_code_for_binary_right(EQF, EQ, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Ne) {
                // not equal !=
                match(Ne);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Lt);

                //*++text = NE;
                emit_code_for_binary_right(NEF, NE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Lt) {
                // less than
                match(Lt);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = LT;
                emit_code_for_binary_right(LTF, LT, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Gt) {
                // greater than
                match(Gt);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = GT;
                emit_code_for_binary_right(GTF, GT, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Le) {
                // less than or equal to
                match(Le);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = LE;
                emit_code_for_binary_right(LEF, LE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
            
                expression(Shl);

                //*++text = GE;
                emit_code_for_binary_right(GEF, GE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Shl) {
                // shift left
                match(Shl);
                int save_type = expr_type;

                *++text = PUSH;
                expression(Add);

                // 两侧的操作数只能是char以及int型的
                if ((save_type == FLOAT || save_type == DOUBLE) ||
                    (expr_type == FLOAT || save_type == DOUBLE)){
                   printf("%d: wrong type argument to shift left\n", line);
                   exit(-1); 
                }

                *++text = SHL;
                
                expr_type = INT;
            }
            else if (token == Shr) {
                // shift right
                match(Shr);
                int save_type = expr_type;

                *++text = PUSH;
                expression(Add);

                // 两侧的操作数只能是char以及int型的
                if ((save_type == FLOAT || save_type == DOUBLE) ||
                    (expr_type == FLOAT || save_type == DOUBLE)){
                   printf("%d: wrong type argument to shitf right\n", line);
                   exit(-1); 
                }

                *++text = SHR;
                
                expr_type = INT;
            }
            //TODO 先尝试让浮点的加法操作正常工作 
            else if (token == Add) {
                // add
                match(Add);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);

                //计算表达式右边的值
                expression(Mul);
                
                printf("+ right type %d\n", expr_type);
                //TODO expr_type = tmp;
                //如果操作数是指针类型的话
                if (expr_type > PTR) { 
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }

                emit_code_for_binary_right(ADDF, ADD, &reserve1, &reserve2);

            }
            else if (token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;

                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;

                    expr_type = tmp;
                }
            }
            else if (token == Mul) { // multiply
                match(Mul);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Inc);

                emit_code_for_binary_right(MULF, MUL, &reserve1, &reserve2);
                //*++text = MUL;

                //TODO
                //expr_type = tmp;
            }
            else if (token == Div) {
                // divide
                match(Div);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                expression(Inc);

                emit_code_for_binary_right(DIVF, DIV, &reserve1, &reserve2);
                //*++text = DIV;

                //expr_type = tmp;
            }
            else if (token == Mod) {
                // Modulo
                match(Mod);

                int save_type = expr_type;
                *++text = PUSH;

                expression(Inc);
                // 只有两个数是整型数(CHAR或INT)才可以
                if (!((save_type == INT || save_type == CHAR) &&
                      (expr_type == INT || expr_type == CHAR))){
                     printf("%d:invalid operands to binary\n", line);
                     exit(-1); 
                }

                *++text = MOD;

                expr_type = INT;
                //expr_type = tmp;
            }
            else if (token == Inc || token == Dec) {
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                //SC store char; SI store int
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);
            }

            //数组的访问,但是好像没有数组的声明
            else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH; //将var的值作为地址放在栈中
                expression(Assign);
                match(']');

                //什么时候需要将type保存到tmp
                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH; //xx的结果放在栈中(计算偏移量)
                    *++text = IMM; 
                    *++text = sizeof(int);
                    *++text = MUL; 
                }
                else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                expr_type = tmp - PTR;
                *++text = ADD; //计算地址:首地址 + 偏移量

                //a[10] 等价于 *(a + 10)
                //LC load char; LI load int
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else {
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
            }
        }
    }
}



static void statement() {

    int *a, *b; 

    if (token == If) {
        // 为if语句产生的汇编代码，不像gcc等正规编译器会进行一系列的优化操作
        // if (...) <statement> [else <statement>]
        //                     //按照顺序来
        //   if (...)           <cond>  
        //                      JZ a    
        //     <statement>      <statement>
        //   else:              JMP b //跳过else部分
        // a:
        //     <statement>      <statement>
        // b:                   b:
        //
        
        match(If);
        match('(');
        //解析条件
        expression(Assign);  
        match(')');

        // 如果结果是float类型的，那么将bx中的数转型移到ax中
        // 转型的精度损失不会影响条件的真假性
        if (expr_type == FLOAT || expr_type == DOUBLE){
               *++text = BTOA;
         }

        *++text = JZ;
        b = ++text; //先为标号b分配一个地址空间

        //解析if中的语句
        statement(); //跳过这些细节     

        int offset;
        //解析else部分
        if (token == Else) { 
            //match包含了next操作, 如果有else if那么statement()后就会匹配if
            match(Else);

            // emit code for JMP B
            // TODO 这里需要重定位location: b, offset: text+3 - text_start
            offset = (text + 3 - text_start)*sizeof(int);
            add_relocation_item(b, offset, Text_Rel);
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            statement(); //跳过这些细节
        }


        // TODO 这里需要重定位location: b, offset: text+1 - text_start
        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //编译完后再填充标号b的内容
    }


    //TODO 实现break, continue
    else if (token == While) {
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1; //a开始将存放<cond>

        match('(');
        expression(Assign);
        match(')');

        // 如果结果是float类型的，那么将bx中的数转型移到ax中
        // 转型的精度损失不会影响条件的真假性
        if (expr_type == FLOAT || expr_type == DOUBLE){
              *++text = BTOA;
        }

        *++text = JZ;
        b = ++text; //先为标号b分配一个地址空间

        //TODO 将两个标号打包压入堆栈中（主要是为了while循环）
        //如果堆栈为空的时候，即此时的环境不是在while循环中，那么报错
        //start_label1: ,  end_label2: 
        //start_label2: ,  end_label2: 
        
        statement();

        int offset;

        //相当于continue
        *++text = JMP;
        //TODO 这里需要一个重定位location:text, offset=a - text_start 
        *++text = (int)a;
        offset = (a - text_start)*sizeof(int);
        add_relocation_item(text, offset, Text_Rel);


        //相当于break
        //编译完后在填充标号b的内容, b开始存放其它命令
        //TODO 这里也需要一个重定位location:b, offset=text+1-text_start

        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //b开始存放其它命令
    }

    //匹配if/while中的语句
    else if (token == '{') {
        // { <statement> ... }
        match('{');

        while (token != '}') {
            statement();
        }

        match('}');
    }

    else if (token == Return) {
        // return [expression];
        match(Return);

        if (token != ';') {
            expression(Assign);
        }

        match(';');

        // emit code for return
        *++text = LEV;
    }

    else if (token == ';') {
        // empty statement
        match(';');
    }

    else {
        // a = b; or function_call();
        //printf("assignement\n");
        expression(Assign);
        match(';');
    }
}



static void global_declaration() 
{
    // 解析变量声明的类型
    if (token == Int) {
        match(Int);
        basetype = INT;
        printf("Int token\n");
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
        printf("Char token\n");
    }
    else if (token == Float){
        match(Float);
        basetype = FLOAT;
        printf("Float token\n");
    }


    // 解析可由逗号分割的变量声明 
    while (token != ';' && token != '}') {
        int final_type = basetype;

        // 解析指针类型，因为会存在如 "int ****var;" 的多级指针声明需要用一个循
        // 环来解析，注意因为在词法分析阶段解析标识符的时候遇到非标识符的字符
        // 就会停止下来，因此 "int**** var;" 这种形式也是可以的
        while (token == Mul) {
            match(Mul);
            final_type = final_type + PTR;
        }

        if (token != Id) {
            // 如果记号不是标识符的话则为非法声明
            printf("%d: bad global declaration\n", line);
            exit(-1);
        }
        if (current_id[Class]) {
            // 标识符已经存在
            printf("%d: duplicate global declaration\n", line);
            exit(-1);
        }
    
        match(Id);

        //设置了Type和Value，等程序后面引用的时候就能正确加载
        current_id[Type] = final_type;

        if(token == Brak){
            //TODO 新增支持数组声明, 数组下标要是整数      
            static int* addr_keeper;
            next();
            if (token != Num){
               printf("%d: bad index\n", line);
            }
            int num = token_val;

            current_id[Class] = Glo;
            //为什么current_id[Value] = (int)data就不行,可能数组的访问就是用指针
            //实现的 data是char*类型的

            //数组变量是指针变量，数组的起始地址保存在该指针变量中
            //这里不能使用(int)&data因为data是全局变量，其数值后面是会变化的
            //因此需要另外一个变量来
            //从data中先分配一个空间用于存放下面数组的首地址
            addr_keeper = (int*)data;
            data = data + sizeof(int);
            *addr_keeper = (int)data;

            current_id[Value] = (int)addr_keeper;
           // current_id[Value] = (int)&data;
           // printf("saved value %p\n",&data);
            current_id[Type] = final_type + PTR;
            
            int* array_addr = (int*)data;
            printf("array addr  %p\n", array_addr);
            data = data + num * sizeof(int);
            printf("new data addr  %p\n", data);

            match(Num);
            match(']');

            
            //TODO 新增支持数组的初始化
            if (token == Assign){
                //like int array[4] = {1,3,5,6};
                int i;
                
                match(Assign);
                match('{');
                for (i=0; i<num; i++){
                   if (token != Num){
                       printf("%d: bad initailzer\n", line);
                   }

                   printf("token_val is %d\n", token_val);
                   printf("address %p\n", array_addr+i);
                   array_addr[i] = token_val;
                   match(Num);
                   if (token == ',') {
                      match(',');
                   }else if (token == '}'){
                      match('}');
                      break;
                   }else{
                       printf("%d: bad token\n", line);
                   }
                }
                if (token == '}'){
                   match('}');
                }
            }

        }else {

            //TODO 根据变量的类型不同分配不同大小空间的
            current_id[Class] = Glo; 
            current_id[Value] = (int)data; 
           
            //新增的代码支持初始化
            if (token == Assign){
               // 如果复杂的话就去掉初始化
               // 例如 int a = 10;
               next();
               if (token != Num){
                  printf("%d: bad initailzer\n", line);
               }

               // 根据变量类型存储相应的值
               if (basetype == CHAR || basetype == INT){
                   *(int*)data = (num_type == INT) ? token_val
                                                   : (int)token_val_float;
               }else if (basetype == FLOAT || basetype == DOUBLE){
                   *(double*)data = (num_type == FLOAT) ? token_val_float
                                                        : (double)token_val;
               }else{
                   // TODO 指针的赋值
               }


               //注意只有初始化的时候才需要匹配Num
               match(Num);
            }

            //更新data地址，按照变量的类型
            if ((basetype == INT)  || 
                (basetype == CHAR) ||
                (final_type > PTR)){
                data = data + sizeof(int);
            }else{
                // 内部的float类型以及double类型的都按照double的类型存储
                data = data + sizeof(double);
            }
        }

        if (token == ',') {
            match(',');
        }
    }

    next();
}

static void parse_configuration()
{ 
    char* token_name;
    
    //use
    next();
    token_name = (char*)current_id[Name];
    if (strncmp("use", token_name, 3)){
       printf("bad use block\n");
       return;
    }
    next();
    match('{');
    while (token != '}') {
        //printf("token %d golbal decalration\n", token);
        global_declaration();
    }
    match('}');

    //action
    token_name = (char*)current_id[Name];
    if (strncmp("action", token_name, 6)){
       printf("bad action block\n");
       return;
    }

    next();
    match('{');
    while (token != '}') {
        //printf("token %d golbal decalration\n", token);
        statement();
    }
    match('}');

}


//只初始化一次
int init()
{
    int i, fd;
    int *tmp;

    poolsize = 256 * 1024; // arbitrary size
    line = 1;


    //编译之后应该只保存text data
    // allocate memory
    if (!(text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    text_start = text;
    //printf("old code start %p\n", text_start);
    
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    data_start = data;
    //printf("old data start %p\n", data_start);


    //运行是会需要，该部分只要虚拟机运行就行了
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    //只是编译的时候会使用 
    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    old_text = text;
}


//每次编译新的代码片段的时候都需要重新设置一下符号表
//TODO 这些是公共的部分应该只初始化一次
static  void init_symbol_table()
{
    //有专门的符号表
    memset(symbols, 0, poolsize);

    //注意这个顺序要和symbol.h中的对应起来，否则回报错误
    //TODO 因为这个是公用的，可以考虑一下将其存放到另一个全局变量中
    //然后和symbol.h的放在一起，让相关的东西在一起方便以后修改
    char* keyword = "char int float if else while return";

    prepare_for_tokenize(keyword, symbols);

    int i;

    //关键字
    i = Char;
    while (i <= Return) {
        next();
        current_id[Token] = i++;
        //printf("store %d\n", current_id[Token]);
    }

    // 解析src中的符号并将其加入到当前标识中，即不需要
    // 这个步骤可以作为单独的函数提炼出来，方便后面加入新的符号
    char* libfunc = "open read close printf malloc memset memcmp exit void";
    prepare_for_tokenize(libfunc, symbols);

    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = FLOAT;
        current_id[Value] = i++;
        //为什么不设置Token的值
        //这些是标识符其token号为133
    } 

    next(); current_id[Token] = Char; // handle void type
    //next(); idmain = current_id; // keep track of main
}


//从外面注入依赖符号
//TODO 如果要处理多个变量的话就建议使用参数包
int* dependency_inject
(
   struct dependency_items* dep_itemsp,   
   const char* src_code
)
{

   init_symbol_table();


   int num_dep_items = dep_itemsp->num_items;
   int i;
   printf("add dependency\n");
   struct dependency* items = dep_itemsp->items;
   for (i=0; i<num_dep_items; i++){
      src = items[i].var_name;
      prepare_for_tokenize(src, symbols);
      next();
      current_id[Class] = Ext;
      current_id[Type] = items[i].var_type;
      current_id[Value] = (int)items[i].var_addr;
   }

        
   //设置代码的起始地址
   code_start = text + 1;
   
   //解析源代码
   prepare_for_tokenize(src_code, symbols);
   parse_configuration();

   //手动添加退出代码
   *++text = EXIT;

   return relocation();

   //return code_start;
}

void dump_text(int* text, int len)
{
    int i;
    printf("dumping text\n");
    for (i=0; i<len-1; i++) {
       printf("%0x ", text[i]);
    }
    
       printf("%0x\n", text[i]);
}


//计算text和data段的长度
//为后面的重定位进行准备
int* relocation()
{
    //text当前地址是使用了的，而data当前地址是未使用的
    actual_text_len = text - text_start + 1;
    actual_data_len = data - data_start;

    //进行字节边界对齐
    //printf("actual_text_len %d\n", actual_text_len);
    printf("actual_data_len %d\n", actual_data_len);
    
    //注意text是int为单位的，data是char为单位的
    int* new_text = malloc(actual_text_len*sizeof(int));
    char* new_data = malloc(actual_data_len*sizeof(char));
    memset(new_text, 0, actual_text_len);
    memset(new_data, 0, actual_data_len);
    //printf("new_text %p, new_data %p\n", new_text, new_data);
    dump_text((int*)text_start, actual_text_len);

    memcpy(new_data, (void*)data_start, actual_data_len);
    
    do_relocation(new_text, new_data);

    memcpy(new_text, (void*)text_start, actual_text_len*sizeof(int));
    //dump_text((int*)new_text, actual_text_len);

    //重置
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    data = (char*)data_start;
    text = (int*)text_start;
    
    return new_text + 1;
}

static int emit_store_directive(int type)
{
   // 如果变量类型是指针类型的，先将保存其地址
   // 最后等expr_type减为其基本类型时再用正确的
   // 存储指令保存相应类型的值
   if (type > PTR) return SI;

   return  (type == CHAR) ? SC : 
           (type == INT) ? SI : 
           (type == FLOAT) ? SF: SD;
}

static int emit_load_directive(int type)
{
   // 如果变量类型是指针类型的，先将加载其地址
   // 最后等expr_type减为其基本类型时再用正确的
   // 加载指令加载相应类型的值
   if (type > PTR) return LI;

   // 处理基本类型char int float double
   return  (type == CHAR) ? LC : 
           (type == INT) ? LI :
           (type == FLOAT) ? LF : LD;
   
}

static int type_of_token(int token)
{
    return (token == Char) ? CHAR : 
           (token == Int) ? INT :
           (token == Float) ? FLOAT : DOUBLE; 

}

static void load_float_constant(double float_const)
{
    //加载浮点常量
    double* addr;

    *++text = FIMM;
    addr = (double*)(text + 1);
    *addr = float_const;

    //内部浮点数常量使用double类型存储占两个字节
    //变量的话有float和double类型的
    text += 2;
}

static void load_int_constant(int int_const)
{
    *++text = IMM;
    *++text = int_const;
}


static int get_base_type(int type)
{
    // CHAR INT FLOAT DOUBLE PTR
    // 因为PTR的值最大，其它非基本类型都是4个基本类型加上若干个
    // PTR得到的，因此可以通过取摸来去除指针类型得到基本类型
    return (type % PTR); 
}


static Boolean does_operate_on_constant()
{
  // TODO 这个条件只是必要条件，即如果一个操作符号
  // 正在操作一个常量的话，那么在该函数被调用的时候
  // 命令序列应该满足下面的条件，但是还没有找到充分
  // 条件
  return (*(text-1) == IMM || *(text-2) == FIMM);
}


static void emit_code_for_binary_left
(
   int** reserve1,
   int** reserve2
)
{
    printf("+ left type %d\n", expr_type);
    if (expr_type == FLOAT || expr_type == DOUBLE){
      //将加载到bx的数压人到fsp栈中
        *++text = PUSF;
     }else{
      //如果后面的表达式的类型是浮点的话，需要修改指令
        *++text = NOP;
        *reserve1 = text;
        *++text = PUSH; 
        *reserve2 = text;
     }        
}

static void emit_code_for_binary_right
(
   int operator_for_float,
   int operator_for_int,
   int** reserve1,
   int** reserve2
)
{
     printf("+ right type %d\n", expr_type);
     if (expr_type == FLOAT || expr_type == DOUBLE){
         //左边的表达式如果是整型的话需要使用ATOB将ax的值转换
         //成double类型存放bx中, 指令修改如下
         if (*reserve1 != NULL){
              *(*reserve1) = ATOB;
              *(*reserve2) = PUSF;
           }

           expr_type = DOUBLE;
           *++text = operator_for_float;  
      }else{
          //前面的是浮点，后面是整型
          if (*reserve1 == NULL){
               //直接将ax的数值转型并存放在bx中，前面的操作数已经压人
               //fsp栈中了
               *++text = ATOB; 
               *++text = operator_for_float;  
               expr_type = DOUBLE;
           }else{
            //两个操作数类型都是整型的
               *++text = operator_for_int;  
               expr_type = INT;
          }
     }
}

static void check_assignment_types(int left_type, int right_type)
{
    if (left_type == right_type) return;

    // 赋值左边是浮点型，右边是整型
    if ((left_type == FLOAT || left_type == DOUBLE) &&
        (right_type == INT || right_type == CHAR)){

            *++text = ATOB; 
        
     }else if ((left_type == INT || left_type == CHAR) &&
              (right_type == FLOAT || right_type == DOUBLE)){
    // 赋值左边是整型，右边是浮点型

            *++text = BTOA; 
     }else{
        //TODO 其它的类型检查
        printf("%d: different types in assignment\n", line);
        exit(-1);
     }
}

