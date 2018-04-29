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
 * ȷ���˱��ʽ������expr_type
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
    // ���ʽ��BNF��ʽ
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
            //TODO ��һ���ж��Ƿ��Ǹ�������

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
            //�ַ�����������Ҫ�ض�λ
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

            //��������
            if (token == '(') {
                match('(');

                //ʵ�εĸ���
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
                    // ϵͳ����
                    *++text = id[Value];
                }
                else if (id[Class] == Fun) {
                    // function call
                    // �Զ���ĺ���
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

                //����������
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
                // enum variable
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else {

                // �������
                // TODO ���ֻ֧��ȫ��������Ļ��������ɾ��
                if (id[Class] == Ext) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]���Ǳ������ַ
                }
                else if (id[Class] == Glo) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]���Ǳ������ַ
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
            //��Ҫ�������ȼ����ߵı��ʽ��ֵ
            expression(Inc); // get the address of
            
            //&*hello ������
            if (*text == LC || *text == LI) {
                //����
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
            //��Ҫ�������ȼ����ߵı��ʽ��ֵ
            expression(Inc);

            //Ȼ����ݱ��ʽ�Ľṹ�����ж�
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
            expression(Inc); //������ڼĴ�����ax
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

                // ��������ǳ䵱��ֵ�����޸�ָ�ʹ��PUSHָ������ַ
                // �����������ֵ�Ļ�����ʹ��Loadָ�����
                if (*text == LC || *text == LI || *text == LF || *text == LD) {   
                    *text = PUSH; 
                } else {
                // ��ֵ���Ǳ���������
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                expression(Assign);

                expr_type = tmp;

                //TODO ����Float���͵�ָ��
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

            //����ķ���,���Ǻ���û�����������
            else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH; //��var��ֵ��Ϊ��ַ����ջ��
                expression(Assign);
                match(']');

                //ʲôʱ����Ҫ��type���浽tmp
                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH; //xx�Ľ������ջ��(����ƫ����)
                    *++text = IMM; 
                    *++text = sizeof(int);
                    *++text = MUL; 
                }
                else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                expr_type = tmp - PTR;
                *++text = ADD; //�����ַ:�׵�ַ + ƫ����

                //a[10] �ȼ��� *(a + 10)
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
        // Ϊif�������Ļ����룬����gcc����������������һϵ�е��Ż�����
        // if (...) <statement> [else <statement>]
        //                     //����˳����
        //   if (...)           <cond>  
        //                      JZ a    
        //     <statement>      <statement>
        //   else:              JMP b //����else����
        // a:
        //     <statement>      <statement>
        // b:                   b:
        //
        
        match(If);
        match('(');
        //��������
        expression(Assign);  
        match(')');

        *++text = JZ;
        b = ++text; //��Ϊ���b����һ����ַ�ռ�

        //����if�е����
        statement(); //������Щϸ��     

        int offset;
        //����else����
        if (token == Else) { 
            //match������next����, �����else if��ôstatement()��ͻ�ƥ��if
            match(Else);

            // emit code for JMP B
            // TODO ������Ҫ�ض�λlocation: b, offset: text+3 - text_start
            offset = (text + 3 - text_start)*sizeof(int);
            add_relocation_item(b, offset, Text_Rel);
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            statement(); //������Щϸ��
        }


        // TODO ������Ҫ�ض�λlocation: b, offset: text+1 - text_start
        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //��������������b������
    }


    //TODO ʵ��break, continue
    else if (token == While) {
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1; //a��ʼ�����<cond>

        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text; //��Ϊ���b����һ����ַ�ռ�

        //TODO ��������Ŵ��ѹ���ջ�У���Ҫ��Ϊ��whileѭ����
        //�����ջΪ�յ�ʱ�򣬼���ʱ�Ļ���������whileѭ���У���ô����
        //start_label1: ,  end_label2: 
        //start_label2: ,  end_label2: 
        
        statement();

        int offset;

        //�൱��continue
        *++text = JMP;
        //TODO ������Ҫһ���ض�λlocation:text, offset=a - text_start 
        *++text = (int)a;
        offset = (a - text_start)*sizeof(int);
        add_relocation_item(text, offset, Text_Rel);


        //�൱��break
        //��������������b������, b��ʼ�����������
        //TODO ����Ҳ��Ҫһ���ض�λlocation:b, offset=text+1-text_start

        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //b��ʼ�����������
    }

    //ƥ��if/while�е����
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

    // ������������������
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

        //������Type��Value���ȳ���������õ�ʱ�������ȷ����
        current_id[Type] = type;

        if(token == Brak){
            //TODO ����֧����������, �����±�Ҫ������      
            static int* addr_keeper;
            next();
            if (token != Num){
               printf("%d: bad index\n", line);
            }
            int num = token_val;

            current_id[Class] = Glo;
            //Ϊʲôcurrent_id[Value] = (int)data�Ͳ���,��������ķ��ʾ�����ָ��
            //ʵ�ֵ� data��char*���͵�

            //���������ָ��������������ʼ��ַ�����ڸ�ָ�������
            //���ﲻ��ʹ��(int)&data��Ϊdata��ȫ�ֱ���������ֵ�����ǻ�仯��
            //�����Ҫ����һ��������
            //��data���ȷ���һ���ռ����ڴ������������׵�ַ
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

            
            //TODO ����֧������ĳ�ʼ��
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

            //TODO ���ݱ��������Ͳ�ͬ���䲻ͬ��С�ռ��
            current_id[Class] = Glo; 
            current_id[Value] = (int)data; 
           
            //�����Ĵ���֧�ֳ�ʼ��
            if (token == Assign){
               // ������ӵĻ���ȥ����ʼ��
               // ���� int a = 10;
               next();
               if (token != Num){
                  printf("%d: bad initailzer\n", line);
               }

               //printf("num_type %d\n", num_type);
               *(int*)data = token_val;

               //ע��ֻ�г�ʼ����ʱ�����Ҫƥ��Num
               match(Num);
            }

            //����data��ַ�����ձ���������
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


//ֻ��ʼ��һ��
int init()
{
    int i, fd;
    int *tmp;

    poolsize = 256 * 1024; // arbitrary size
    line = 1;


    //����֮��Ӧ��ֻ����text data
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


    //�����ǻ���Ҫ���ò���ֻҪ��������о�����
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    //ֻ�Ǳ����ʱ���ʹ�� 
    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    old_text = text;
}


//ÿ�α����µĴ���Ƭ�ε�ʱ����Ҫ��������һ�·��ű�
//TODO ��Щ�ǹ����Ĳ���Ӧ��ֻ��ʼ��һ��
static  void init_symbol_table()
{
    //��ר�ŵķ��ű�
    memset(symbols, 0, poolsize);

    //ע�����˳��Ҫ��symbol.h�еĶ�Ӧ����������ر�����
    //TODO ��Ϊ����ǹ��õģ����Կ���һ�½����ŵ���һ��ȫ�ֱ�����
    //Ȼ���symbol.h�ķ���һ������صĶ�����һ�𷽱��Ժ��޸�
    char* keyword = "char int float if else while return";

    prepare_for_tokenize(keyword, symbols);

    int i;

    //�ؼ���
    i = Char;
    while (i <= Return) {
        next();
        current_id[Token] = i++;
        //printf("store %d\n", current_id[Token]);
    }

    // ����src�еķ��Ų�������뵽��ǰ��ʶ�У�������Ҫ
    // ������������Ϊ�����ĺ������������������������µķ���
    char* libfunc = "open read close printf malloc memset memcmp exit void";
    prepare_for_tokenize(libfunc, symbols);

    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
        //Ϊʲô������Token��ֵ
        //��Щ�Ǳ�ʶ����token��Ϊ133
    } 

    next(); current_id[Token] = Char; // handle void type
    //next(); idmain = current_id; // keep track of main
}


//������ע����������
//TODO ���Ҫ�����������Ļ��ͽ���ʹ�ò�����
int* dependency_inject
(
   char* sym, 
   void *extern_addr,
   const char* src_code
)
{

   init_symbol_table();

   prepare_for_tokenize(sym, symbols);
   //Ŀǰֻ����һ�����ŵĵ���
   src = sym;
   next();

   //�ֶ����÷��ű������ⲿ���ŵ��빤��     
   current_id[Class] = Ext;
   current_id[Type] = DOUBLE;
   current_id[Value] = (int)extern_addr;
        
   //���ô������ʼ��ַ
   code_start = text + 1;
   
   //����Դ����
   prepare_for_tokenize(src_code, symbols);
   parse_configuration();

   //�ֶ�����˳�����
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


//����text��data�εĳ���
//Ϊ������ض�λ����׼��
int* relocation()
{
    //text��ǰ��ַ��ʹ���˵ģ���data��ǰ��ַ��δʹ�õ�
    actual_text_len = text - text_start + 1;
    actual_data_len = data - data_start;

    //�����ֽڱ߽����
    //printf("actual_text_len %d\n", actual_text_len);
    printf("actual_data_len %d\n", actual_data_len);
    
    //ע��text��intΪ��λ�ģ�data��charΪ��λ��
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

    //����
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    data = (char*)data_start;
    text = (int*)text_start;
    
    return new_text + 1;
}


