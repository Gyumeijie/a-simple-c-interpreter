#include "lex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char* src;
static int  *symbols;

// 标识符的描述信息 
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

int *current_id;
int token;
// 保存int char等整型数
int integral_token_val;
// 保存float double的浮点类型数
double real_token_val;
int line;
int num_type;
extern char* data;

void prepare_for_tokenization(const char* src_code, int* symbol_table)
{
    src = src_code;
    symbols = symbol_table;
}

void next() {
    char *last_pos;
    int hash;

    while (token = *src) {
        ++src;

        if (token == '\n') {
            ++line;
        }

        else if (token == '#') {
            //跳过宏定义，因为不支持
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        
        //解析标识符
        else if (is_valid_identifier_leading_character(token)) {

            last_pos = (char*)src - 1;
            hash = token;

            while (is_valid_identifier_character(*src)) {
                hash = hash * 147 + *src;
                src++;
            }
           
            // 搜索符号表
            // 这里默认设置的IdSize即标识符的长度是10，如果两个符号的前面10个是
            // 相同的，那么就区分不出来了,可以根据实际情况下重新设置其大小
            current_id = symbols;
            int id_len = src - last_pos;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && 
                    !memcmp((char *)current_id[Name], last_pos, id_len)) {
                    token = current_id[Token];

                    return;
                }
                //查找下一个条目
                current_id = current_id + IdSize;
            }

            //如果没有找到就在新的symbols表项中创建一个ID条目
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;

            return;
        }
        
        // 如果是字面量的话就计算其数值
        else if (token >= '0' && token <= '9') {

            // 保存浮点数字面量，之后用转换函数进行转换
            char float_string[64];
            const char* string_begin = src;

            // 这里注意一些十进制单独0的情况
            integral_token_val = token - '0';
            num_type = INT;
            if (integral_token_val > 0) {
                float_string[0] = token;
                int idx = 1;

                // 十进制
                while (*src >= '0' && *src <= '9') {
                    integral_token_val = integral_token_val*10 + *src++ - '0';
                }

                // 检测是否可能是浮点，即检测下一个字符是否是'.'
                // 对于浮点数暂时不支持如0001.xxx的浮点数形式
                if (*src == '.'){
                    memcpy(&float_string[1], string_begin, src - string_begin);
                    idx = idx + src - string_begin;
                    float_string[idx] = '.';
                    
                    process_fraction(float_string, idx + 1);

                    real_token_val = strtod(float_string, NULL);
                    num_type = FLOAT;
                }

            } else {
                // '0'开头的数，八进制或者十六进制或者是小数
                if (*src == 'x' || *src == 'X') {
                    // 十六进制
                    token = *++src;
                    int sum = 0;
                    while ((token >= '0' && token <= '9') || 
                           (token >= 'a' && token <= 'f') || 
                           (token >= 'A' && token <= 'F')) {
                        sum = sum*16 + digitalize_hex_character((char)token);
                        token = *++src;
                    }
                    integral_token_val = sum;

                }else if(*src == '.'){
                    // 小数0.xxxxxx形式 
                    float_string[0] = '0';
                    float_string[1] = '.';

                    process_fraction(float_string, 2);
            
                    real_token_val = strtod(float_string, NULL);
                    num_type = FLOAT;
                }else{
                    // 八进制用的比较少暂时不支持
                }
            }

            token = Num;
            return;
        }

        else if (token == '.'){
           //处理.xxxxx形式的浮点数
           char float_string[32];
           float_string[0] = '.';
           process_fraction(float_string, 1);
         
           real_token_val = strtod(float_string, NULL);
           token = Num;
           num_type = FLOAT;
           return;
        }

        else if (token == '/') {
            if (*src == '/') {
                //跳过注释 
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else { 
                token = Div;
                return;
            }
        }

        else if (token == '"') {
            // 解析字符串常量，目前只支持转义字符'\n', 字符串常量的值存放在data
            // 段
            last_pos = data;

            //存取字符字面量
            while (*src != 0 && *src != token) {
                integral_token_val = *src++;
                // 处理字符串中的转义字符
                if (integral_token_val == '\\') {
                    integral_token_val = *src++;
                    if (integral_token_val == 'n') {
                        integral_token_val = '\n';
                    }
                }

                //存放字符串常量中的字符
                *data++ = integral_token_val;
            }

            src++;
            integral_token_val = (int)last_pos;

            return;
        }

        else if (token == '\''){
            integral_token_val = *src++;
        
            //处理单引号中的转义字符
            if (integral_token_val == '\\'){
                integral_token_val = *src++;
                if (integral_token_val == 'n') {
                     integral_token_val = '\n';
                }
            }

            //单引号中只能有一个转义字符（两个字符）和一个非转义字符,如果还有其
            //它的字符则报错
            if (*src != '\''){
               printf("%d: bad char value\n", line);
               exit(-1);
            }

            src++;
            // 如'c', 就返回Num，token_val以赋值为相应的ascii值
            token = Num; 

            return;
        }

        else if (token == '=') {
            // 解析 '==' 和 '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // 解析 '+' 和 '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // 解析 '-' 和 '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // 解析'!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // 解析 '<=', '<<' or '<'
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
            //解析'>='，'>>' 或者 '>'
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
            //解析'|'和'||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            //解析'&'和'&&'
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
        else if (token == '~' || 
                 token == ';' || 
                 token == '{' || 
                 token == '}' || 
                 token == '(' || 
                 token == ')' ||
                 token == ']' || 
                 token == ',' ||
                 token == ':') {
            //直接将这些字符作为token返回 
            return;
        }
        else{
           //其它情况忽略
        }
    }
}


void match(int expected_token) {
    if (token == expected_token) {
        next();
    } else {
        printf("%d: expected token: %d\n", line, expected_token);
        exit(-1);
    }
}


static Boolean is_valid_identifier_leading_character(char ch)
{

    if ( (ch >= 'a' && ch <= 'z') ||
         (ch >= 'A' && ch <= 'Z') ||
         (ch == '_')){ 
           return True;
         }

    return False;
}


static Boolean is_valid_identifier_character(char ch)
{

    if (is_valid_identifier_leading_character(ch) || is_digit(ch)){
        return True;
    }

    return False;
}


static Boolean is_digit(char ch)
{
    return (ch >= '0' && ch <= '9') ? True : False;
}


//处理浮点数的小数部分
static void process_fraction(char* float_string, int start_idx)
{
   int idx = start_idx;

   token = *++src;
   while ((token >= '0' && token <= '9')){ 
       float_string[idx] = token;
       idx++;
       token = *++src;
   }
        
   //判断是否是非法的浮点数字面量，处理完正常部分的浮点数后如果后面不是这些字符
   //的话，那么这个浮点字面量是非法的，同时也能处理上面出现非法字符的情况，例如
   //"12.0a" 这样的字面量
   //printf("trailing charater of float literal '%c'\n", token);
   if (! (token == ',' || token == ';' || token == ' ')){
       printf("%d: bad float literal\n", line);
       exit(-1);
    }

    float_string[idx] = '\0';
    printf("float val:%lf\n", strtod(float_string, NULL));
}


//将十六进制的字符转化成相应的数字
static int digitalize_hex_character(char ch)
{
   if ((ch >= '0' && ch <= '9')){
      return ch - '0';      
   }else if ((token >= 'a' && token <= 'f')){
      return ch - 'a' + 10;  
   }else{
      return ch - 'A' + 10;  
   }
}
