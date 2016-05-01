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

/* Assemble from n through lst->tail (exclusive) into buff (buff provided
 * should be large enough to fit the contents). */
static void assemble(struct LinkedList *lst, struct Node *n, char *buff) {
	while ((n = n->next) != lst->tail)
		buff = memcpy(buff, n->payload, n->payload_size) + n->payload_size;
}

/* Destroy the node n and return its next elem */
static inline struct Node *destroy_node(struct Node *n) {
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

/* Append another list onto the end of this one. */
static void append_list(struct LinkedList *lst, struct LinkedList *append) {
	lst->tail = append->head;
	lst->bytes += append->bytes;
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
	(*lst)->assemble = assemble;
	(*lst)->prune  = prune;
	(*lst)->append = append;
	(*lst)->append_list = append_list;

	return lst;
}
