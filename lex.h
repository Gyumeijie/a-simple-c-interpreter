#ifndef LEX_H
#define LEX_H

#include "symbol.h"

extern void prepare_for_tokenize(const char* src_code, int* symbol_table);
extern void next();
extern void match(int tk); 

#endif
