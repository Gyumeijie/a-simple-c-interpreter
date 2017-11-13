#include <stdio.h>
#include "parser.h"
#include "executor.h"

int main()
{
   init();
   interpreter_init();
   int test_extern;
   char* src;
   char* dependency;

   
   /*
   src = "use{int i = 1; int j = 1;} action{printf(\"test_extern is  %d\n\", test_extern + i + j);}";
   dependency = "test_extern";
   int* code1 = dependency_inject(dependency, &test_extern, src);

   test_extern = 34;
   run_code(code1);

   src = "use{int i = 2; int k = 3;} action{printf(\"test_extern is %d\n\", test_extern + i*k);}";
   dependency = "test_extern";
   int* code2 = dependency_inject(dependency, &test_extern, src);

   test_extern = 35;
   run_code(code2);
   */


   src = "use{int i = 3; int j = 4;} action{if(i > j){test_extern = 100 - i + j;} else if (i == 2){test_extern = 100;} else{test_extern = 0; while (i--) { test_extern++;}} }";
   dependency = "test_extern";
   int* code2 = dependency_inject(dependency, &test_extern, src);
   run_code(code2);
   printf("test_extern is %d\n", test_extern);

}
