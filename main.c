#include <stdio.h>
#include "parser.h"
#include "executor.h"
#include "dependency.h"

int main()
{
   parser_init();
   executor_init();

   int data = 13; 
   int result;
   struct dependency_items* dep_itemsp;
   dep_itemsp = init_dependency_items(2);
   add_dependency_item(dep_itemsp, "data", &data, INT);
   add_dependency_item(dep_itemsp, "result", &result, INT);

   char* src = "use{} action{result = data * 6;}";
   
   int* code1 = compile_src_code(dep_itemsp, src);
   run_code(code1);

   printf("result is %d\n",  result);
   printf("result %p, data %p:", &result, &data);
 
   printf("IMM:%0x PUSH:%0x LI:%0x MUL:%0x  SI:%0x  ADD:%0x EXIT:%0x\n", IMM, PUSH, LI, MUL, SI, ADD, EXIT);

}
