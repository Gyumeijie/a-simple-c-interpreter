#include "relocation.h"
#include "symbol.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>


int* relocation
(
   int* old_text_start,
   int* old_text_end,
   char* old_data_start,
   char* old_data_end
)
{
    // text当前地址是使用了的，而data当前地址是未使用的
    int actual_text_len = old_text_end - old_text_start + 1;
    int actual_data_len = old_data_end - old_data_start;
    
    // 注意text是int为单位的，data是char为单位的
    int* new_text = malloc(actual_text_len * sizeof(int));
    char* new_data = malloc(actual_data_len * sizeof(char));
    memset(new_text, 0, actual_text_len);
    memset(new_data, 0, actual_data_len);

    memcpy(new_data, (void*)old_data_start, actual_data_len);
    
    do_relocation(new_text, new_data);

    memcpy(new_text, (void*)old_text_start, actual_text_len * sizeof(int));
 
    // 对于text段的第一个单元未使用是一个bug，后面如果有时间可以尝试去解决
    return new_text + 1;
}


void add_relocation_item
(
    int *text_location, 
    int offset, 
    int kind
)
{
    struct relocation_item item = {text_location, offset, kind};
    relocation_items[cur_put_item] = item;

    cur_put_item++;
    num_rel_items++;
    
}


static void do_relocation
(   
   const int* new_text_addr, 
   const char* new_data_addr
)
{
   int i;
   for (i=0; i<num_rel_items; i++){
      int *text_location = relocation_items[i].text_location;
      int offset = relocation_items[i].offset;
      int kind = relocation_items[i].kind;

      // 虽然data是以char为单位的, text是以int为单位的当把它们转型成
      // 整型数offset的计算理应是不一样的，但是在添加重定位项目的时候
      // 已经考虑它们的单位的差异性了，这里可以统一使用offset，而不用
      // 乘上相应的单位(data是1，text是4)
      if (kind == Data_Rel){
         *text_location = ((int)new_data_addr + offset);
      }else{
         *text_location = ((int)new_text_addr + offset);
      }
   }

   // 为重定位下一个代码段初始化
   reset_relocation_items();
}


static void reset_relocation_items()
{
   num_rel_items = 0;
   cur_put_item = 0;
}
