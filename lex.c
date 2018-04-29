#include "lex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char* src;
static int  *symbols;

// ��ʶ����������Ϣ 
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

// �������ߺ������� 
enum { CHAR, INT, FLOAT, PTR};

//�������� 
enum {Global, Local, Extern};


int *current_id;
int token;
int token_val;
//�������������渡������
double token_val_float;
int line;
int num_type;
extern char* data;

void prepare_for_tokenize(const char* src_code, int* symbol_table)
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
            //�����궨�壬��Ϊ��֧��
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        
        //������ʶ��
        else if (is_valid_starting_character(token)) {

            last_pos = (char*)src - 1;
            hash = token;

            char block_keyword[32];
            while (is_valid_identifier_character(*src)) {
                hash = hash * 147 + *src;
                src++;
            }
           
            // �������ű�
            // ����Ĭ�����õ�IdSize����ʶ���ĳ�����10������������ŵ�ǰ��10����
            // ��ͬ�ģ���ô�����ֲ�������,���Ը���ʵ������������������С
            current_id = symbols;
            int id_len = src - last_pos;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && 
                    !memcmp((char *)current_id[Name], last_pos, id_len)) {
                    token = current_id[Token];

                    return;
                }
                //������һ����Ŀ
                current_id = current_id + IdSize;
            }

            //���û���ҵ������µ�symbols�����д���һ��ID��Ŀ
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;

            return;
        }
        
        //������������Ļ��ͼ�������ֵ
        //���ڸ�������ʱ��֧����0001.xxx�ĸ�������ʽ
        else if (token >= '0' && token <= '9') {
            num_type = INT;

            //���渡������������֮����ת����������ת��
            char float_string[32];
            const char* string_begin = src;

            token_val = token - '0';
            if (token_val > 0) {
                float_string[0] = token;
                int idx = 1;

                // ʮ����
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
                
                //����Ƿ�����Ǹ���
                if (*src == '.'){
                    memcpy(&float_string[1], string_begin, src - string_begin);
                    idx = idx + src - string_begin;
                    float_string[idx] = '.';
                    
                    process_fraction(float_string, idx + 1);

                    token_val_float = strtod(float_string, NULL);
                    num_type = FLOAT;
                }

            } else {
                // '0'��ͷ�������˽��ƻ���ʮ�����ƻ�����С��
                if (*src == 'x' || *src == 'X') {
                    // ʮ������
                    token = *++src;
                    int sum = 0;
                    while ((token >= '0' && token <= '9') || 
                           (token >= 'a' && token <= 'f') || 
                           (token >= 'A' && token <= 'F')) {
                        sum = sum*16 + digitalize_hex_character((char)token);
                        token = *++src;
                    }
                    token_val = sum;

                //TODO ���Ӹ�������
                }else if(*src == '.'){
                    // С��0.xxxx 
                    float_string[0] = '0';
                    float_string[1] = '.';

                    process_fraction(float_string, 2);
            
                    token_val_float = strtod(float_string, NULL);
                    num_type = FLOAT;
                }else{
                    // �˽��� 
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }

        else if (token == '.'){
           //����.xxxxx��ʽ�ĸ�����
           char float_string[32];
           float_string[0] = '.';
           process_fraction(float_string, 1);
         
           token_val_float = strtod(float_string, NULL);
           token = Num;
           num_type = FLOAT;
           return;
        }

        else if (token == '/') {
            if (*src == '/') {
                //����ע�� 
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else { 
                token = Div;
                return;
            }
        }

        else if (token == '"') {
            // �����ַ���������Ŀǰֻ֧��ת���ַ�'\n', �ַ���������ֵ�����data
            // ��
            last_pos = data;

            //��ȡ�ַ�������
            while (*src != 0 && *src != token) {
                token_val = *src++;
                // �����ַ����е�ת���ַ�
                if (token_val == '\\') {
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                //����ַ��������е��ַ�
                *data++ = token_val;
            }

            src++;
            token_val = (int)last_pos;

            return;
        }

        else if (token == '\''){
            token_val = *src++;
        
            //���������е�ת���ַ�
            if (token_val == '\\'){
                token_val = *src++;
                if (token_val == 'n') {
                     token_val = '\n';
                }
            }

            //��������ֻ����һ��ת���ַ��������ַ�����һ����ת���ַ�,���������
            //�����ַ��򱨴�
            if (*src != '\''){
               printf("%d: bad char value\n", line);
               exit(-1);
            }

            src++;
            // ��'c', �ͷ���Num��token_val�Ը�ֵΪ��Ӧ��ascii
            token = Num; 

            return;
        }

        else if (token == '=') {
            // ���� '==' �� '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // ���� '+' �� '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // ���� '-' �� '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // ����'!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // ���� '<=', '<<' or '<'
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
            //����'>='��'>>' ���� '>'
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
            //����'|'��'||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            //����'&'��'&&'
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
            // directly return the character as token;
            return;
        }
        else{
           //�����������
        }
    }
}


void match(int tk) {
    if (token == tk) {
        next();
    } else {
        printf("%d: expected token: %d\n", line, tk);
        exit(-1);
    }
}

static Boolean is_valid_starting_character(char ch)
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

    if (is_valid_starting_character(ch) || is_digit(ch)){
        return True;
    }

    return False;
}

static Boolean is_digit(char ch)
{
    return (ch >= '0' && ch <= '9') ? True : False;
}


//����������С������
static void process_fraction(char* float_string, int start_idx)
{
   int idx = start_idx;

   token = *++src;
   while ((token >= '0' && token <= '9')){ 
       float_string[idx] = token;
       idx++;
       token = *++src;
   }
        
   //�ж��Ƿ��ǷǷ��ĸ��������������������������ֵĸ�������������治����Щ�ַ�
   //�Ļ�����ô��������������ǷǷ��ģ�ͬʱҲ�ܴ���������ַǷ��ַ������������
   //"12.0a" ������������
   //printf("trailing charater of float literal '%c'\n", token);
   if (! (token == ',' || token == ';' || token == ' ')){
       printf("%d: bad float literal\n", line);
       exit(-1);
    }

    float_string[idx] = '\0';
    printf("float val:%lf\n", strtod(float_string, NULL));
}


//��ʮ�����Ƶ��ַ�ת������Ӧ������
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
