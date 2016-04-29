#include <stdlib.h>

#include "linked_list.h"

/*********** Private Utils ***********/
static struct Node *empty_node(void) {
	struct Node *empty;

	if(!(empty = malloc(sizeof struct Node)))
		return NULL;

	empty->payload_size = 0;
	empty->payload = NULL;
	empty->next = NULL;

	return empty;
}

/* Destroy the node n and return its next elem */
static inline struct Node *destroy_node(struct Node *n) {
{
	struct Node *next = n->next;
	free(n->payload);
	free(n);

	return next;
}

/*********** Public API ***********/
/* Remove from lst->head through n (exclusive) */
static void prune(struct LinkedList *lst, struct Node *n) {
	struct Node *cur = lst->head;
	lst->head = n;

	while(n != (cur = destroy_node(cur)));
}

/*
 * Append node to the head of the list. The list grows to the left.
 * empty_node <-. <-. <-.
 *      ^       ^       ^
 *      |       |       |
 *     tail   head_n   head
 *
 * Users of the list can track it with their own head_n. They can track any
 * item in the list, including the empty_node (makes it so that all non empty
 * nodes can be freed without having users point to freed memory).
 *
 * Returns a pointer to the new empty node (new tail) or NULL if unsuccessful.
 */
static struct Node *append(struct LinkedList *lst, char *payload, long psize) {
	struct *empt;

	if (!(*empt = empty_node()))
		return NULL;

	/* Fill out the empty node. */
	lst->tail->payload = payload;
	lst->tail->payload_size = psize;
	lst->tail->next = empt;

	/* Now, move the tail to the new empty node */
	lst->tail = empt;

	/* Increase the by count */
	lst->bytes += psize;

	return OK;
}

struct LinkedList *linked_list_init() {
	/* We use an empty node as the head so that way when we hand out references
	 * to the node the references will be correct whenever the node is
	 * populated */
	struct Node *empty_node;

	if(!(lst = malloc(sizeof struct LinkedList)))
		return NULL;

	if (!empty_node()) {
		free(lst);
		return NULL;
	}

	(*lst)->head = empty_node;
	(*lst)->tail = empty_node;
	(*lst)->bytes = 0;

	/* Methods */
	(*lst)->append = append;
	(*lst)->prune  = prune;

	return lst;
}
