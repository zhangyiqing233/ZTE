#include <stdio.h>
#include <stdlib.h>
#include<string.h>

typedef struct Rule
{
    int a[5];                       //目的ip
    int b[5];                       //源ip
    unsigned short int c[2];        //目的端口
    unsigned short int d[2];        //源端口
    unsigned char e[2];             //协议
    unsigned char result;           //结果
}RULE, *PRULE;

typedef struct Msg
{
    int a[4];                        //目的ip
    int b[4];                        //源ip
    unsigned short int c;            //目的端口
    unsigned short int d;            //源端口
    unsigned char e;                 //协议
}MSG, *PMSG;

typedef struct Data
{
    unsigned int a;
    unsigned int b;
    unsigned short int c;
    unsigned short int d;
    unsigned char e;
    unsigned char result;
}DATA, *PDATA;

typedef struct Mask
{
    unsigned int a;
    unsigned int b;
    unsigned short int c;
    unsigned short int d;
    unsigned char e;
}MASK, *PMASK;

typedef struct Node      //建立链表存储每次处理出来的数据
{
    DATA data;
    MASK mask;
    struct Node * next;
}NODE, *PNODE;

typedef struct Range
{
    unsigned short int data;
    unsigned short int mask;
    struct Range * next;
}RANGE, *PRANGE;

void append(PNODE* ptail, PNODE node)
{
    (*ptail)->next = node;
    while((*ptail)->next)
    {
        *ptail = (*ptail)->next;
    }
}

PRANGE range_match(short int lo, short int hi)
{
    PRANGE port_head = (PRANGE)malloc(sizeof(RANGE));
    PRANGE port_tail = (PRANGE)malloc(sizeof(RANGE));
    port_head->next = port_tail;
    port_tail->next = NULL;
    PRANGE cur = port_tail;
    cur->next = NULL;
    port_head->data = lo;
    port_tail->data = hi;
    port_head->mask = 0xffff;
    port_tail->mask = 0xffff;
    unsigned short int a = 1, b = 0xffff, flag = (lo ^ hi);
    int count = 0, i = 0;
    while(flag)
    {
        flag >>= 1;
        count++;
    }
    while(--count > 0)
    {
        if((lo & a) == 0)
        {
            PRANGE pnew = (PRANGE)malloc(sizeof(RANGE));
            pnew->data = (lo & b) + a;
            pnew->mask = b;
            cur->next = pnew;
            pnew->next = NULL;
            cur = pnew;
        }
        if(((hi & a) >> i) == 1)
        {
            PRANGE pnew = (PRANGE)malloc(sizeof(RANGE));
            pnew->data = (hi & b) - a;
            pnew->mask = b;
            cur->next = pnew;
            pnew->next = NULL;
            cur = pnew;
        }
        a <<= 1;
        b <<= 1;
        i++;
    }
    return port_head;
}

PNODE deal_rule(PRULE rule)
{
    PDATA data = (PDATA)malloc(sizeof(DATA));
    PMASK mask = (PMASK)malloc(sizeof(MASK));
    data->a = ((rule->a[0] << 24) | (rule->a[1] << 16) | (rule->a[2] << 8) | rule->a[3] );
    mask->a = (-1 >> (32 - rule->a[4])) << (32 - rule->a[4]);                             //通过移位操作解决前缀匹配精度问题
    data->b = ((rule->b[0] << 24) | (rule->b[1] << 16) | (rule->b[2] << 8) | rule->b[3] );
    mask->b = (-1 >> (32 - rule->b[4])) << (32 - rule->b[4]);
    data->e = rule->e[0];
    mask->e = rule->e[1];
    data->result = rule->result;

    PNODE head = (PNODE)malloc(sizeof(NODE));
    head->next = NULL;

    PNODE cur = head;
    cur->next = NULL;
    PRANGE c_range = range_match(rule->c[0], rule->c[1]);
    PRANGE d_range = range_match(rule->d[0], rule->d[1]);
    PRANGE prc = c_range;
    PRANGE prd = d_range;

    while(prc)
    {
        data->c = prc->data;
        mask->c = prc->mask;
        while(prd)
        {
            data->d = prd->data;
            mask->d = prd->mask;
            PNODE node = (PNODE)malloc(sizeof(NODE));
            node->data = *data;
            node->mask = *mask;
            cur->next = node;
            node->next = NULL;
            cur = node;
            prd = prd->next;
        }
        prd = d_range;
        prc = prc->next;
    }
    /*for(i = rule->c[0]; i <= rule->c[1]; i++)
    {
        data->c = i;
        mask->c = 65535;
        for(j = rule->d[0]; j <= rule->d[1]; j++)
        {
            data->d = j;
            mask->d = 0xffff;
            PNODE node = (PNODE)malloc(sizeof(NODE));
            node->data = *data;
            node->mask = *mask;
            cur->next = node;
            node->next = NULL;
            cur = node;
        }
    }*/
    return head->next;
}

int msg_match(PNODE head, PMSG msg)
{
    PNODE cur = head;
    while (cur != NULL)
    {
        if((((msg->a[0] << 24) | (msg->a[1] << 16) | (msg->a[2] << 8) | msg->a[3] ) & cur->mask.a) == (cur->data.a & cur->mask.a)

            && ((((msg->b[0] << 24) | (msg->b[1] << 16) | (msg->b[2] << 8) | msg->b[3] ) & cur->mask.b) == (cur->data.b & cur->mask.b))

                && ((msg->c & cur->mask.c) == cur->data.c)

                    && ((msg->d & cur->mask.d) == cur->data.d)

                        && (msg->e == cur->data.e))

                           return cur->data.result;
        else
            cur = cur->next;
    }
    return 0;
}

int main()
{
    FILE *pf, *op;
    pf = fopen("E:\\in.txt", "r");
    op = fopen("E:\\out.txt", "w");
    int n1, n2;
    while(~fscanf(pf, "%d", &n1))
    {
        PNODE head = (PNODE)malloc(sizeof(NODE));
        head->next = NULL;
        PNODE tail = head;
        tail->next = NULL;
        int i, sum = 0;
        for (i = 0; i < n1; i++)
        {
            PRULE rule = (PRULE)malloc(sizeof(RULE));
            fscanf(pf,"%d.%d.%d.%d/%d", &rule->a[0], &rule->a[1], &rule->a[2], &rule->a[3], &rule->a[4]);
            fscanf(pf,"%d.%d.%d.%d/%d", &rule->b[0], &rule->b[1], &rule->b[2], &rule->b[3], &rule->b[4]);
            fscanf(pf,"%hu:%hu", &rule->c[0], &rule->c[1]);        //把整数作为unsigned short int 十进制类型读取
            fscanf(pf,"%hu:%hu", &rule->d[0], &rule->d[1]);
            fscanf(pf,"%hhx/%hhx", &rule->e[0], &rule->e[1]);     //把整数作为unsigned char 十进制类型读取
            fscanf(pf,"%hhu", &rule->result);
            append(&tail, deal_rule(rule));
        }

        PNODE cur = head->next;
        while(cur)
        {
            sum++;
            cur = cur->next;
        }

        fprintf(op, "32 32 16 16 8 %d\n", sum);
        cur = head->next;
        while(cur)
        {
            fprintf(op, "data:%#x %#x %#hx %#hx %#hhx %hhu\nmask:%#x %#x %#hx %#hx %#hhx\n",
                   cur->data.a, cur->data.b, cur->data.c, cur->data.d, cur->data.e, cur->data.result,
                   cur->mask.a, cur->mask.b, cur->mask.c, cur->mask.d, cur->mask.e);
            cur = cur->next;
        }

        fscanf(pf, "%d", &n2);
        for (i = 0; i < n2; i++)
        {
            PMSG msg = (PMSG)malloc(sizeof(MSG));
            fscanf(pf, "%d.%d.%d.%d", &msg->a[0], &msg->a[1], &msg->a[2], &msg->a[3]);
            fscanf(pf, "%d.%d.%d.%d", &msg->b[0], &msg->b[1], &msg->b[2], &msg->b[3]);
            fscanf(pf, "%hu", &msg->c);
            fscanf(pf, "%hu", &msg->d);
            fscanf(pf, "%hhu", &msg->e);
            fprintf(op, "%d\n", msg_match(head->next, msg));
        }
    }

    return 0;
}
