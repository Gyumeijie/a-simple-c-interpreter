#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "symbol.h"

#define STACK_SIZE 1024

extern int executor_init();
extern void run_code(int* code_start);

static int eval(int* pc, int* sp, double* fsp);
#endif
