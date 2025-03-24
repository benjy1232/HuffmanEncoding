//
// Created by bserrano on 2/15/2025.
//

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct Node {
	struct Node* next;
	void* data;
};

struct LinkedList {
	struct Node* head;
	struct Node* tail;
};


void llist_free(struct LinkedList* llist, void (*cb)(void*));

bool llist_pushback(struct LinkedList* llist, void* data);
void* llist_popfront(struct LinkedList* llist);

/* Essentially a Priority Queue */
bool llist_insertUsingCompare(struct LinkedList* llist, void* data, bool (*compareFunc)(void*, void*));

#endif //LIST_H
