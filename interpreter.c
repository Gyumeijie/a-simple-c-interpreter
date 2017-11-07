#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "interpreter.h"



static void next() {
    char *last_pos;
    int hash;

    while (token = *src) {
        ++src;

        if (token == '\n') {
            if (assembly) {
                // print compile info
                printf("%d: %.*s", line, src-old_src, old_src);
                old_src = src;

                while (old_text < text) {
                    printf("%8.4s", & "LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,"
                                      "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                                      "OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT"[*++old_text * 5]);

                    if (*old_text <= ADJ)
                        printf(" %d\n", *++old_text);
                    else
                        printf("\n");
                }
            }
            ++line;
        }

        else if (token == '#') {
            // skip macro, because we will not support it
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        

        //������ʶ��
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {

            // parse identifier
            last_pos = src - 1;
            hash = token;

            char block_keyword[32];
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }
           

            // look for existing identifier, linear search
            // �������ű�
            // ����Ĭ�����õ�IdSize����ʶ���ĳ�����10������������ŵ�ǰ��10����
            // ��ͬ�ģ���ô�����ֲ�������,���Ը���ʵ������������������С
            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
                    //found one, return
                    //printf("find token %d\n", current_id[Token]);
                    token = current_id[Token];
                    return;
                }
                //������һ����Ŀ
                current_id = current_id + IdSize;
            }

            // store new ID
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        }
        
        //TODO ���Ӹ�����������Ҳ������ζ��Ҫ
        //������������Ļ��ͼ�������ֵ
        else if (token >= '0' && token <= '9') {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            token_val = token - '0';
            if (token_val > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
            } else {
                // starts with number 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }


        else if (token == '/') {
            if (*src == '/') {
                // skip comments
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                // divide operator
                token = Div;
                return;
            }
        }
        else if (token == '"' || token == '\'') {
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into data.
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    // escape character
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            // if it is a single character, return Num token
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }

            return;
        }
        else if (token == '=') {
            // parse '==' and '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // parse '!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                token = Lan;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
}


static void match(int tk) {
    if (token == tk) {
        next();
    } else {
        printf("%d: expected token: %d\n", line, tk);
        exit(-1);
    }
}


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

            // emit code
            *++text = IMM;
            *++text = token_val;

            expr_type = INT;
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
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));

            expr_type = PTR;
        }
        else if (token == Sizeof) {
            // sizeof is actually an unary operator
            // now only `sizeof(int)`, `sizeof(char)` and `sizeof(*...)` are
            // supported.
            match(Sizeof);
            match('(');
            expr_type = INT;

            if (token == Int) {
                match(Int);
            } else if (token == Char) {
                match(Char);
                expr_type = CHAR;
            }

            while (token == Mul) {
                match(Mul);
                expr_type = expr_type + PTR;
            }

            match(')');

            // emit code
            *++text = IMM;
            *++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);

            expr_type = INT;
        }

        //�����ʶ��������Ҫ�����ʶ����Ӧ��[id]Value
        else if (token == Id) {
            // there are several type when occurs to Id
            // but this is unit, so it can only be
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable
            match(Id);
            id = current_id;

            if (token == '(') {
                // function call
                match('(');

                // pass in arguments
                tmp = 0; // number of arguments
                while (token != ')') {
                    expression(Assign);
                    *++text = PUSH;
                    tmp ++;

                    if (token == ',') {
                        match(',');
                    }

                }
                match(')');

                // emit code
                if (id[Class] == Sys) {
                    // system functions
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
                if (id[Class] == Loc) {
                    *++text = LEA;
                    *++text = index_of_bp - id[Value]; //�����*pc++
                }
                else if (id[Class] == Glo) {
                    *++text = IMM;                
                    //TODO �����ʱ�����Ҫע��һ���ض���ı���
                    *++text = id[Value]; //id[Value]���Ǳ������ַ
                }
                else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }

                // emit code, default behaviour is to load the value of the
                // address which is stored in `ax`

                expr_type = id[Type];
                //TODO Char�ǲ���д����Ӧ����CHAR
                *++text = (expr_type == CHAR) ? LC : LI;
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
        
        //TODO ��Ӷ�continue�Լ�break���Ĵ���

    }

 
    // binary operator and postfix operators.
    {
        while (token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) {
                    *text = PUSH; // save the lvalue's pointer
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
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
    // there are 8 kinds of statements here:
    // 1. if (...) <statement> [else <statement>]
    // 2. while (...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    // 6. expression; (expression end with semicolon)

    int *a, *b; // bess for branch control

    if (token == If) {
        // Ϊif�������Ļ����룬����gcc����������������һϵ�е��Ż�����
        // if (...) <statement> [else <statement>]
        //
        //   if (...)           <cond>
        //                      JZ a
        //     <statement>      <statement>
        //   else:              JMP b
        // a:
        //     <statement>      <statement>
        // b:                   b:
        //
        //
        match(If);
        match('(');
        expression(Assign);  // parse condition
        match(')');

        // emit code for if
        *++text = JZ;
        b = ++text; //��Ϊ���b����һ����ַ�ռ�

        statement();         // parse statement
        if (token == Else) { // parse else
            match(Else);

            // emit code for JMP B
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            statement();
        }

        //��������������b������
        *b = (int)(text + 1);
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

        *++text = JZ; //��Ϊ���b����һ����ַ�ռ�
        b = ++text;    

        //TODO ��������Ŵ��ѹ���ջ�У���Ҫ��Ϊ��whileѭ����
        //�����ջΪ�յ�ʱ�򣬼���ʱ�Ļ���������whileѭ���У���ô����
        //start_label1: ,  end_label2: 
        //start_label2: ,  end_label2: 

        statement();

        //�൱��continue
        *++text = JMP;
        *++text = (int)a;

        //�൱��break
        //��������������b������, b��ʼ�����������
        *b = (int)(text + 1); 
    }

    
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

/*
void enum_declaration() {
    // parse enum [id] { a = 1, b = 3, ...}
    int i;
    i = 0;
    while (token != '}') {
        if (token != Id) {
            printf("%d: bad enum identifier %d\n", line, token);
            exit(-1);
        }
        next();
        if (token == Assign) {
            // like {a=10}
            next();
            if (token != Num) {
                printf("%d: bad enum initializer\n", line);
                exit(-1);
            }

            i = token_val;
            next();
        }

        current_id[Class] = Num;
        current_id[Type] = INT;
        current_id[Value] = i++;

        if (token == ',') {
            next();
        }
    }
}*/


static void function_parameter() {
    int type;
    int params;
    params = 0;
    while (token != ')') {
        // int name, ...
        type = INT;
        if (token == Int) {
            match(Int);
        } else if (token == Char) {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        // parameter name
        if (token != Id) {
            printf("%d: bad parameter declaration\n", line);
            exit(-1);
        }
        if (current_id[Class] == Loc) {
            printf("%d: duplicate parameter declaration\n", line);
            exit(-1);
        }

        match(Id);
        // store the local variable
        current_id[BClass] = current_id[Class]; current_id[Class]  = Loc;
        current_id[BType]  = current_id[Type];  current_id[Type]   = type;
        current_id[BValue] = current_id[Value]; current_id[Value]  = params++;   // index of current parameter

        if (token == ',') {
            match(',');
        }
    }
    index_of_bp = params+1;
}

static void function_body() {
    match('}');
    // type func_name (...) {...}
    //                   -->|   |<--

    // ����һ��Ҫ�����ǰ��
    // ... {
    // 1. local declarations
    // 2. statements
    // }

    int pos_local; // position of local variables on the stack.
    int type;
    pos_local = index_of_bp;

    //�������ر�������
    while (token == Int || token == Char) {
        // local variable declaration, just like global ones.
        basetype = (token == Int) ? INT : CHAR;
        match(token);

        while (token != ';') {
            type = basetype;
            //ָ�����ͣ� �����Ƕ༶ָ��
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }

            //֮���Ƿ���
            if (token != Id) {
                // invalid declaration
                printf("%d: bad local declaration\n", line);
                exit(-1);
            }

            //����Ƿ��ظ�����
            if (current_id[Class] == Loc) {
                // identifier exists
                printf("%d: duplicate local declaration\n", line);
                exit(-1);
            }
            match(Id);


            // store the local variable
            current_id[BClass] = current_id[Class]; current_id[Class]  = Loc;
            current_id[BType]  = current_id[Type];  current_id[Type]   = type;
            current_id[BValue] = current_id[Value]; current_id[Value]  = ++pos_local;   // index of current parameter

            if (token == ',') {
                match(',');
            }
        }
        match(';');
    }

    // save the stack size for local variables
    // ENT enter
    *++text = ENT;
    *++text = pos_local - index_of_bp;

    //��ʼ�������
    // statements
    while (token != '}') {
        statement();
    }

    // emit code for leaving the sub function
    // LEV leave
    *++text = LEV;
}


static void function_declaration() {
    // type func_name (...) {...}
    //               | this part

    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();
    //match('}');


    // unwind local variable declarations for all local variables.
    current_id = symbols;
    while (current_id[Token]) {
        if (current_id[Class] == Loc) {
            current_id[Class] = current_id[BClass];
            current_id[Type]  = current_id[BType];
            current_id[Value] = current_id[BValue];
        }
        current_id = current_id + IdSize;
    }
}


static void global_declaration() {
    // int [*]id [; | (...) {...}]


    int type; // tmp, actual type for variable
    int i; // tmp 

    basetype = INT;

    /*
    // parse enum, this should be treated alone.
    if (token == Enum) {
        // enum [id] { a = 10, b = 20, ... }
        match(Enum);
        if (token != '{') {
            match(Id); // skip the [id] part
        }
        if (token == '{') {
            // parse the assign part
            match('{');
            enum_declaration();
            match('}');
        }

        match(';');
        return;
    }*/

    // parse type information
    if (token == Int) {
        match(Int);
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
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

        if (token == '(') {
            current_id[Class] = Fun;
            //�����ĵ�ַ
            current_id[Value] = (int)(text + 1); // the memory address of function
            function_declaration();

        }else if(token == Brak){
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
            current_id[Class] = Glo; 
            //TODO ���ݱ��������Ͳ�ͬ���䲻ͬ��С�ռ��
            //text���еı����ĵ�ַ���ض�λ��ʱ����Ҫ����Ҫ�ı�
            //�ڽ������ĵ�ַ��ֵ����Ҫע������ⲿ����ķ������ַ��
            //text�����ǲ���Ҫ�޸ĵģ������Ҫһ���������ֱ����Ļ���
            //��Ϊһ��ѡ�����ⲿ���ŵ�ʱ��curren_id[class] ����ΪImpt
            //��ô�ڽ��� *++text = current_id[Value]��ʱ��ͬʱע��һ���ض����
            //����ȴ��ض����ʱ��������Щ�ռ�������
            current_id[Value] = (int)data; 
           
            //�����Ĵ���֧�ֳ�ʼ��
            if (token == Assign){
                //like int a = 10;
               next();
               if (token != Num){
                  printf("%d: bad initailzer\n", line);
               }

               *(int*)data = token_val;
            }

            data = data + sizeof(int);
            match(Num);
        }

        if (token == ',') {
            match(',');
        }
    }

    next();
}

//���ļ������Ͻ�������
//ȫ�ֱ����ͺ�������
static void program() {
    // get next token
    next();
    while (token > 0) {
        printf("token %d golbal decalration\n", token);
        global_declaration();
    }
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


static int eval() {
    int op, *tmp;
    cycle = 0;
    while (1) {
        cycle ++;
        //TODO ����main������ʱ���Ǵ�main������ʼִ�еģ����Ҫȥ��main�����Ļ�
        //��Ҫ��ȷ����pc����ͻ��ڴ����
        op = *pc++; 

        //TODO ʹ��switch ������Ч��if/else�жϣ���Ϊ�ڵ��Ե�ʱ����Ҫ����ĳ��
        //op��ʱ�� ���op�ܺ�����ôǰ�����Ҫ���кܶ��if/else�������ж�
 
        if (op == IMM)       {ax = *pc++;}                                     // load immediate value to ax
        else if (op == LC)   {ax = *(char *)ax;}                               // load character to ax, address in ax
        else if (op == LI)   {ax = *(int *)ax;}                                // load integer to ax, address in ax
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}                       // save character to address, value in ax, address on stack
        else if (op == SI)   {*(int *)*sp++ = ax;}                             // save integer to address, value in ax, address on stack
        //��ջ������������
        else if (op == PUSH) {*--sp = ax;}                                     // push the value of ax onto the stack
        else if (op == JMP)  {pc = (int *)*pc;}                                // jump to the address
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}                   // jump if ax is zero
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}                   // jump if ax is zero
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;}           // call subroutine
        //else if (op == RET)  {pc = (int *)*sp++;}                              // return from subroutine;
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}      // make new stack frame
        else if (op == ADJ)  {sp = sp + *pc++;}                                // add esp, <size>
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;}  // restore call frame and PC
        else if (op == LEA)  {ax = (int)(bp + *pc++);}                         // load address for arguments.

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


        //�ṩ��Ҫ��һЩ��������
        //ֻҪ������Ӧ��op����ִ���ض��Ķ���������
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp);}
        //�ǲ���printfֻ�ܴ���6������
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
    
        //������ṩ˽�еĺ������̣�Ϊ�˲�����Ϊ����ר�õĺ������̵�����ִ����
        //��switch���̫�󣬾ͽ���Щ˽�е��Ƶ�

        //1xxx xxxx ˽�еĴ����ʾ���� 
        // op = PRIV_OP - 128 �ٵ��þ����һ�������� 
        // private_executor(text, data, op){
        //     
        // }
        

        //Ψһ�˳��Ĵ���
        else if (op == EXIT) { printf("exit(%d)\n", *sp); return *sp;}
        else {
            printf("unknown instruction:%d\n", op);
            return -1;
        }

    }
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
    
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }

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
static void init_symbol_table()
{
    //��ר�ŵķ��ű�
    memset(symbols, 0, poolsize);

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void";

    int i;
    //�ؼ���
    // add keywords to symbol table
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
        //printf("store %d\n", current_id[Token]);
    }

    // add library to symbol table
    // ����src�еķ��Ų�������뵽��ǰ��ʶ�У�������Ҫ
    // ������������Ϊ�����ĺ������������������������µķ���
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;

        //���ַ������ض�λ��ʱ��Ҳ�ǲ���Ҫ���¼����, ��Ϊ���漰��data���ݶ�
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
   int *extern_addr,
   const char* src_code
)
{

   init_symbol_table();

   //Ŀǰֻ����һ�����ŵĵ���
   src = sym;
   next();

   //�ֶ����÷��ű������ⲿ���ŵ��빤��     
   current_id[Class] = Glo;
   current_id[Type] = INT;

   //�ⲿ����Ĳ����������ض�λ��ʱ��Ҳ�Ǻ��Եģ���Ϊ�䲻��data���ݶ�
   current_id[Value] = (int)extern_addr;
        
   //���ô������ʼ��ַ
   code_start = text + 1;
   
   //����Դ����
   src = (char* )src_code;
   parse_configuration();

   //�ֶ�����˳�����
   *++text = EXIT;

   return code_start;
}


void run_code(int* code_start)
{
   //����pc��ֵ
   pc = code_start;

   //��ʼ����ջ
   sp = (int *)((int)stack + poolsize);
   eval();
}



//����text��data�εĳ���
//Ϊ������ض�λ����׼��
static int actual_text_len, text_start;
static int actual_data_len, data_start;
int get_text_and_data_length()
{
    actual_text_len = text_start - (int)text;
    actual_data_len = data_start - (int)data;

    //�����ֽڱ߽����
}


//TODO �ض�λ����Ҫ��������ݲ��ֽ��е�һ���������µ�λ��һ������
//���ݶ�ֱ�ӿ���, ������ȿ��������޸�(�������޸��ٿ���)
int relocation()
{


}
