#ifndef RELOCATION_H
#define RELOCATION_H

//TODO �������õ�int��Ϊָ��ĸĳ�PointerΪ�˸��õ���ֲ��
struct relocation_item{
    int* text_location;
    int  data_offset;
};

#define MAX_RELOCATION_ITEM  32
static struct relocation_item relocation_items[MAX_RELOCATION_ITEM];

extern void add_relocation_item(int *text_location, int data_offset);
extern void do_relocation(const int* new_data_addr);
static void reset_relocation_items();

static int cur_put_item = 0;
static int num_rel_items = 0;

#endif
