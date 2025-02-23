//
// Created by bserrano on 2/15/2025.
//

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct Node {
    void* data;
    struct Node* next;
};

struct LinkedList {
    struct Node* head;
    struct Node* tail;
};


bool llist_pushback(struct LinkedList* llist, void* data);
void* llist_popfront(struct LinkedList* llist);

bool llist_insertUsingCompare(struct LinkedList* llist, void* data, bool (*compareFunc)(void*, void*));

#endif //LIST_H
