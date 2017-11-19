#ifndef DEPENDENCY_H
#define DEPENDENCY_H

// TODO 如果后面需要增加对注入的依赖其读写进行控制
// 还需要增加一些字段
struct dependency{
  char* var_name;
  void* var_addr;
  int var_type;
};

struct dependency_items{
    int num_items;
    int cur_items;
    struct dependency* items;
};

extern struct dependency_items* init_dependency_items(int num_items);

extern int add_dependency_item
(   
    struct dependency_items* dep_itemsp,
    char* var_name, 
    void* var_addr, 
    int var_type
);

#endif
