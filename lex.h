#ifndef LEX_H
#define LEX_H

#include "symbol.h"
#include "types.h"

extern void prepare_for_tokenize(const char* src_code, int* symbol_table);

extern void next();

extern void match(int tk); 

static Boolean is_valid_starting_character(char ch);

static Boolean is_valid_identifier_character(char ch);

static Boolean is_digit(char ch);

static void process_fraction(char* float_string, int start_idx);

#endif
