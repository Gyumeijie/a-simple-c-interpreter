#include "relocation.h"
#include <stdio.h>

//需要重定向的项在text段的位置以及数据段的偏移量
void add_relocation_item(int *text_location, int data_offset)
{
    struct relocation_item item = {text_location, data_offset};
    relocation_items[cur_put_item] = item;
    printf("text_location %p, data_offset %d\n", text_location, data_offset);
    cur_put_item++;
    num_rel_items++;
    
}


//先修改原有text的重定位项然后在搬移text段,数据段用的是新的
void do_relocation(const int* new_data_addr)
{
   int i;
   for (i=0; i<num_rel_items; i++){
      int *text_location = relocation_items[i].text_location;
      int data_offset = relocation_items[i].data_offset;
      *text_location = ((int)new_data_addr + data_offset);
   }

   reset_relocation_items();
}


static void reset_relocation_items()
{
   num_rel_items = 0;
   cur_put_item = 0;
}
