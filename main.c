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

   src = "use{int i = 1;} action{printf(\" test_extern is  %d\n\", test_extern + i);}";
   dependency = "test_extern";
   int* code1 = dependency_inject(dependency, &test_extern, src);

   src = "use{int i = 2;} action{printf(\" test_extern is  %d\n\", test_extern + i);}";
   dependency = "test_extern";
   int* code2 = dependency_inject(dependency, &test_extern, src);

   test_extern = 35;
   run_code(code2);

   test_extern = 34;
   run_code(code1);


   src = "use{;} action{test_extern = 100 - 1;}";
   dependency = "test_extern";
   code2 = dependency_inject(dependency, &test_extern, src);
   run_code(code2);
   printf("test_extern is %d\n", test_extern);

}
