#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "parser.h"
#include "executor.h"
#include "lex.h"
#include "relocation.h"

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

        if (token == Num) {
            match(Num);
            //TODO 进一步判断是否是浮点类型

            if (num_type == INT){
              // emit code
              *++text = IMM;
              *++text = token_val + 1;
              expr_type = INT;
            }else{
              *++text = FIMM;
              *++text = token_val + 2;
              expr_type = FLOAT;
            }
        }
        else if (token == '"') {
            // continous string "abc" "abc"

            // emit code
            *++text = IMM;
            *++text = token_val;

            match('"');
            // store the rest strings
            while (token == '"') {
                match('"');
            }

            // append the end of string character '\0', all the data are default
            // to 0, so just move data one position forward.
            //字符串常量不需要重定位
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));

            expr_type = PTR;
        }

        else if (token == Id) {
            // there are several type when occurs to Id
            // but this is unit, so it can only be
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable
            match(Id);
            id = current_id;

            //函数调用
            if (token == '(') {
                match('(');

                //实参的个数
                tmp = 0;
                while (token != ')') {
                    expression(Assign);
                    *++text = PUSH;
                    tmp++;

                    if (token == ',') {
                        match(',');
                    }

                }
                match(')');

                if (id[Class] == Sys) {
                    // 系统函数
                    *++text = id[Value];
                }
                else if (id[Class] == Fun) {
                    // function call
                    // 自定义的函数
                    *++text = CALL;
                    *++text = id[Value];
                }
                else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }

                // clean the stack for arguments
                if (tmp > 0) {
                    *++text = ADJ;
                    *++text = tmp;
                }

                //变量的类型
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
                // enum variable
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else {

                // 处理变量
                // TODO 如果只支持全局作用域的话这个可以删掉
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

                // emit code, default behaviour is to load the value of the
                // address which is stored in `ax`

                expr_type = id[Type];

                *++text = (expr_type == CHAR) ? LC : 
                          (expr_type == INT) ? LI: 
                          (expr_type == FLOAT) ? LF : LD;
            }
        }
        else if (token == '(') {
            // cast or parenthesis
            match('(');
            if (token == Int || token == Char) {
                tmp = (token == Char) ? CHAR : INT; // cast type
                match(token);
                while (token == Mul) {
                    match(Mul);
                    tmp = tmp + PTR;
                }

                match(')');

                //cast has precedence as Inc(++)
                expression(Inc); 

                expr_type  = tmp;
            } else {
                // normal parenthesis
                expression(Assign);
                match(')');
            }
        }
        else if (token == Mul) {
            // dereference *<addr>
            match(Mul);
            // dereference has the same precedence as Inc(++)
            expression(Inc); 

            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            *++text = (expr_type == CHAR) ? LC : LI;
        }
        else if (token == And) {
            match(And);
            //需要计算优先级更高的表达式的值
            expression(Inc); // get the address of
            
            //&*hello 抵消了
            if (*text == LC || *text == LI) {
                //回退
                text--;
            }else {
                printf("%d: bad address of\n", line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }
        else if (token == '!') {
            // not
            match('!');
            //需要计算优先级更高的表达式的值
            expression(Inc);

            //然后根据表达式的结构进行判断
            // emit code, use <expr> == 0
            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            expr_type = INT;
        }
        else if (token == '~') {
            // bitwise not
            match('~');
            expression(Inc); //结果放在寄存器中ax
            //(1111 1111)  -1
            //(0110 0011)  XOR
            //______________
            //
            //(1001 1100)
            // emit code, use <expr> XOR -1
            *++text = PUSH; 
            *++text = IMM;  
            *++text = -1;
            *++text = XOR;

            expr_type = INT;
        }
        else if (token == Add) {
            // +var, do nothing
            match(Add);
            expression(Inc);

            expr_type = INT;
        }
        else if (token == Sub) {
            // -var
            match(Sub);

            if (token == Num) {
                *++text = IMM;
                *++text = -token_val;
                match(Num);
            } else {
                *++text = IMM;
                *++text = -1;   //ax
                *++text = PUSH; //*--sp = ax;
                expression(Inc);
                *++text = MUL;  //ax = *sp++ * ax
            }

            expr_type = INT;
        }
        else if (token == Inc || token == Dec) {
            tmp = token;
            match(token);
            expression(Inc);
            if (*text == LC) {
                *text = PUSH;  // to duplicate the address
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
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }
        else {
            printf("%d: bad expression\n", line);
            exit(-1);
        }
    }
 

    // binary operator and postfix operators.
    {
        while (token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);

                // 变量如果是充当左值话就修改指令，使用PUSH指令保存其地址
                // 如果是用作右值的话，就使用Load指令加载
                if (*text == LC || *text == LI || *text == LF || *text == LD) {   
                    *text = PUSH; 
                } else {
                // 左值不是变量，报错
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                expression(Assign);

                expr_type = tmp;

                //TODO 增加Float类型的指令
                *++text = (expr_type == CHAR) ? SC : 
                          (expr_type == INT) ? SI : 
                          (expr_type == FLOAT) ? SF: SD;
            }

            else if (token == Cond) {
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            }

            else if (token == Lor) {
                // logic or
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);

                expr_type = INT;
            }
            else if (token == Lan) {
                // logic and
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);

                expr_type = INT;
            }
            else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;

                expr_type = INT;
            }
            else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;

                expr_type = INT;
            }
            else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                
                expr_type = INT;
            }
            else if (token == Eq) {
                // equal ==
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;

                expr_type = INT;
            }
            else if (token == Ne) {
                // not equal !=
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;

                expr_type = INT;
            }
            else if (token == Lt) {
                // less than
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                
                expr_type = INT;
            }
            else if (token == Gt) {
                // greater than
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;

                expr_type = INT;
            }
            else if (token == Le) {
                // less than or equal to
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;

                expr_type = INT;
            }
            else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;

                expr_type = INT;
            }
            else if (token == Shl) {
                // shift left
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                
                expr_type = INT;
            }
            else if (token == Shr) {
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                
                expr_type = INT;
            }
            else if (token == Add) {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
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
            else if (token == Mul) {
                // multiply
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;

                expr_type = tmp;
            }
            else if (token == Div) {
                // divide
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;

                expr_type = tmp;
            }
            else if (token == Mod) {
                // Modulo
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;

                expr_type = tmp;
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



static void global_declaration() {
    // int [*]id [; | (...) {...}]


    int type; // tmp, actual type for variable
    int i; // tmp 

    basetype = INT;

    // 解析变量声明的类型
    if (token == Int) {
        match(Int);
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
        printf("Char token\n");

    }
    else if (token == Float){
        printf("Float token\n");
        match(Float);
        basetype = FLOAT;
        //basetype = INT;
    }


    // parse the comma seperated variable declaration.
    while (token != ';' && token != '}') {
        type = basetype;

        // parse pointer type, note that there may exist `int ****x;`
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        if (token != Id) {
            // invalid declaration
            printf("%d: bad global declaration\n", line);
            exit(-1);
        }
        if (current_id[Class]) {
            // identifier exists
            printf("%d: duplicate global declaration\n", line);
            exit(-1);
        }
    
        match(Id);

        //设置了Type和Value，等程序后面引用的时候就能正确加载
        current_id[Type] = type;

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
            current_id[Type] = type + PTR;
            
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

               //printf("num_type %d\n", num_type);
               *(int*)data = token_val;

               //注意只有初始化的时候才需要匹配Num
               match(Num);
            }

            //更新data地址，按照变量的类型
            if ((basetype == INT)  || 
                (basetype == CHAR) ||
                (type > PTR)){
                data = data + sizeof(int);
            }else{
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
        current_id[Type] = INT;
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
   char* sym, 
   void *extern_addr,
   const char* src_code
)
{

   init_symbol_table();

   prepare_for_tokenize(sym, symbols);
   //目前只处理一个符号的导入
   src = sym;
   next();

   //手动设置符号表，进行外部符号导入工作     
   current_id[Class] = Ext;
   current_id[Type] = DOUBLE;
   current_id[Value] = (int)extern_addr;
        
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


