#ifndef RELOCATION_H
#define RELOCATION_H

//TODO 将所有用到int作为指针的改成Pointer为了更好的移植性
struct relocation_item{
    int* text_location;
    int  offset;
    int kind;  
};

#define MAX_RELOCATION_ITEM  32
static struct relocation_item relocation_items[MAX_RELOCATION_ITEM];

extern void add_relocation_item(int *text_location, int offset, int kind);
extern void do_relocation(const int* new_text_addr, const char* new_data_addr);
static void reset_relocation_items();

static int cur_put_item = 0;
static int num_rel_items = 0;

#endif
