#ifndef INVADER_LIST
#define INVADER_LIST

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct invader_node{
    int x;
    int y;
    struct invader_node* next;
    struct invader_node* tail;
    bool state;                 //true if it has to move down, false if it can move left right or down
} invader_node;


void delete_invader(invader_node* prev_invader, invader_node* invader);

#endif