#include "dependency.h"
#include <stdlib.h>

/**
 *  因为不同的应用条件下需要注入的依赖数量是可变的，因此需要一个
 *  统一的接口来进行描述，引入dependency_items就是用来解决这个问
 *  题的
 */

struct dependency_items* init_dependency_items(int num_items)
{
   struct dependency_items* dep_itemsp;
   struct dependency* items;
   dep_itemsp = malloc(sizeof(struct dependency_items));
   dep_itemsp->num_items = num_items;
   dep_itemsp->cur_items = 0;
   items = malloc(sizeof(struct dependency) * num_items);
   dep_itemsp->items = items;

   return dep_itemsp;
}


int add_dependency_item
(   
    struct dependency_items* dep_itemsp,
    char* var_name, 
    void* var_addr, 
    int var_type
)
{
    if (dep_itemsp == NULL ||
        dep_itemsp->cur_items == dep_itemsp->num_items){
            return -1;
    }

    struct dependency* dep = &dep_itemsp->items[dep_itemsp->cur_items++];
    dep->var_name = var_name;
    dep->var_addr = var_addr;
    dep->var_type = var_type;

    return 0;
}
