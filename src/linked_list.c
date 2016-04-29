#include <stdlib.h>

#include "linked_list.h"

/* Destroy the node n and return its next elem */
#define destroy_node(n) \
	({ \
		struct Node *next = n->next; \
		free(n->payload); \
		free(n); \
		next; \
	})

static enum RmpgErr init_node(struct Node **n, char *payload, long psize) {
	if(!(*n = malloc(sizeof struct Node)))
		return OUT_OF_MEM;

	(*n)->payload = payload;
	(*n)->payload_size = psize;
	(*n)->next = NULL;

	return OK;
}

/* Insert a new node into the list */
static enum RmpgErr insert_new(struct LinkedList *lst, char *payload, long psize) {
	struct Node *n;

	if(lst->init_node(&n, payload, psize))
		return OUT_OF_MEM;

	lst->insert(lst, n);

	return OK;
}

/* Remove from node a till node b */
static void remove_till(struct Node *a, struct Node *b) {
	a->next = b->next;
}

/*
 * Insert node to the head of the list. The list grows to the left.
 * head -> .-> .-> NULL
 */
static void insert(struct LinkedList *lst, struct Node *n) {
	/* Point the new node at the old head (head could be NULL and that is fine)
	 * */
	n->next = lst->head;
	/* Move the head over */
	lst->head = n;
}

/* Destroy the whole lst */
static void destroy(struct LinkedList *lst) {
	struct Node *n = lst->head;

	/* Iterate the list and destroy each node */
	while (n = destroy_node(n));
}

/* Remove which from the list */
static void remove(struct LinkedList *lst, struct Node *which) {
	struct Node **pp = &lst->head;

	/* Find the node to update and store its address in *pp (could have been
	 * lst->head). */
	while(*pp != which)
		if (!(*pp)->next)
			return;
		else
			*remove = &(*remove)->next

	(*pp)->next = which->next;
	destroy_node(which);
}

static inline char *payload(struct Node *n) {
	return n->payload;
}

static inline struct Node *next(struct Node *n) {
	return n->next;
}

static inline int bytes(struct LinkedList *lst) {
	return lst->bytes;
}

enum RmpgErr linked_list_init(struct LinkedList **lst) {
	if(!(*lst = malloc(sizeof struct LinkedList)))
		return OUT_OF_MEM;

	(*lst)->head = NULL;
	(*lst)->bytes = 0;
	(*lst)->node_init = node_init;

	/* Methods */
	(*lst)->init_node  =  init_node;
	(*lst)->init       =  init;
	(*lst)->insert_new =  insert_new;
	(*lst)->insert     =  insert;
	(*lst)->destroy    =  destroy;
	(*lst)->remove     =  remove;
	(*lst)->bytes      =  bytes;
	(*lst)->next       =  next;
	(*lst)->payload    =  payload;

	return OK;
}
