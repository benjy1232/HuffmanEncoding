//
// Created by bserrano on 2/15/2025.
//
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool llist_pushback(struct LinkedList* llist, void* data)
{
	if (!llist)
	{
		fprintf(stderr, "%s: Unable to use null llist\r\n", __func__);
		return false;
	}

	struct Node* node = malloc(sizeof(struct Node));
	if (!node)
	{
		fprintf(stderr, "%s: Failed to allocate new node\r\n", __func__);
		return false;
	}

	node->data = data;
	node->next = NULL;

	if (!llist->head)
		llist->head = node;
	else
		llist->tail->next = node;
	llist->tail = node;
	return true;
}

bool llist_pushfront(struct LinkedList* llist, void* data)
{
	if (!llist)
	{
		fprintf(stderr, "%s: Unable to use null list\r\n", __func__);
		return false;
	}

	struct Node* node = malloc(sizeof(struct Node));
	if (!node)
	{
		fprintf(stderr, "%s: Failed to allocate new node\r\n", __func__);
		return false;
	}

	node->data = data;
	node->next = llist->head;
	llist->head = node;
	if (!llist->tail)
		llist->tail = node;
}

void* llist_popfront(struct LinkedList* llist)
{
	if (!llist || !llist->head)
		return NULL;

	struct Node* node = llist->head;
	llist->head = llist->head->next;
	void* data = node->data;
	free(node);
	return data;
}

bool node_insert(struct Node* node, void* data)
{
	if (!node)
		return false;

	struct Node* nextNode = malloc(sizeof(struct Node));
	if (!nextNode)
	{
		fprintf(stderr, "%s: Failed to allocate memory for next node\r\n", __func__);
		return false;
	}
	nextNode->next = node->next;
	node->next = nextNode;
	nextNode->data = data;
	return true;
}

bool llist_insertUsingCompare(struct LinkedList* llist, void* elem, bool (*compareFunc)(void*, void*))
{
	if (!llist)
		return false;
	if (!llist->head)
		return llist_pushback(llist, elem);

	bool inserted = false;
	struct Node* iter = llist->head;
	struct Node* prev = NULL;
	while (iter)
	{
		if (compareFunc(iter->data, elem))
		{
			inserted = true;
			if (iter == llist->head)
				return llist_pushfront(llist, elem);
			if (!node_insert(prev, elem))
				return false;
			break;
		}
		prev = iter;
		iter = iter->next;
	}

	if (llist->tail->next != NULL)
		llist->tail = llist->tail->next;

	if (!inserted)
		return llist_pushback(llist, elem);

	return true;
}
