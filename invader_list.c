#include "invader_list.h"

void delete_invader(invader_node* prev_invader, invader_node* invader){

    if(invader == NULL){
        return;
    }

    if(invader->next == NULL){
        invader = NULL;
        return;
    }

    if(prev_invader == NULL){
        invader->next->tail = invader->tail;
        invader = invader->next;                //now first two nodes are the same with correct tail, can delete second node like normal
        delete_invader(invader, invader->next);
    } else{
        prev_invader->next = invader->next;
        free(invader);
    }


}