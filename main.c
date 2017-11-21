#include <stdio.h>
#include "parser.h"
#include "executor.h"
#include "dependency.h"

int main()
{
   parser_init();
   executor_init();
   char* src;
   char* dependency;

   /*
   double test_extern;
   src = "use{int j; float i;} action{j=33; i = 1.3; test_extern = 11.2 + i + 1.5;}";
   dependency = "test_extern";
   int* code1 = dependency_inject(dependency, &test_extern, src);

   test_extern = 34;
   run_code(code1);
   printf("test_extern is %lf\n", test_extern);
   */

   
   double test_extern = 10;
   int  result;

   struct dependency_items* dep_itemsp;
   dep_itemsp = init_dependency_items(2);
   add_dependency_item(dep_itemsp, "test_extern", &test_extern, DOUBLE);
   add_dependency_item(dep_itemsp, "result", &result, INT);
   src = "use{double f;} action{result = (float*)test_extern; }";
   
   int* code1 = dependency_inject(dep_itemsp, src);

   run_code(code1);
   printf("test_extern is  %lf, result is %d\n",  test_extern, result);

   /*
   src = "use{int iii = 2; int k = 3;} action{printf(\"test_extern is %d\n\", test_extern + iii*k);}";
   dependency = "test_extern";
   int* code2 = dependency_inject(dependency, &test_extern, src);

   test_extern = 35;
   run_code(code2);
   */ 


   /*
   src = "use{int i = 3; int j = 4;} action{if(i > j){test_extern = 100 - i + j;} else if (i == 2){test_extern = 100;} else{test_extern = 0; while (i--) { test_extern++;}} }";
   dependency = "test_extern";
   int* code2 = dependency_inject(dependency, &test_extern, src);
   run_code(code2);
   printf("test_extern is %d\n", test_extern);
   */

}
