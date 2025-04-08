//
// Created by bserrano on 2/15/2025.
//

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct Node
{
    struct Node *next;
    void *data;
};

typedef struct
{
    struct Node *head;
    struct Node *tail;
} LinkedList;

void llist_free(LinkedList *llist, void (*cb)(void *));

bool llist_pushback(LinkedList *llist, void *data);
void *llist_popfront(LinkedList *llist);

/* Essentially a Priority Queue */
bool llist_insertUsingCompare(LinkedList *llist, void *data, bool (*compareFunc)(void *, void *));

#endif // LIST_H
