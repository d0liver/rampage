#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "message_q.h"

/*********** Private Utils ***********/
static struct Node *empty_node(void) {
	struct Node *empty;

	if(!(empty = malloc(sizeof (struct Node))))
		return NULL;

	empty->payload_size = 0;
	empty->payload = NULL;
	empty->next = NULL;

	return empty;
}

/* Assemble from n through msg_q->tail (exclusive) into buff (buff provided
 * should be large enough to fit the contents). */
static long assemble(struct MessageQ *msg_q, struct Node *n, char *buff) {
	long bytes = 0;
	char *tmp = buff;

	while (n != msg_q->tail) {
		/* debug("Assemble part: %s\n", n->payload); */
		/* debug("Assemble part size: %ld\n", n->payload_size); */
		if (buff)
			buff = memcpy(buff, n->payload, n->payload_size) + n->payload_size;
		bytes += n->payload_size;
		n = n->next;
	}

	if (buff)
		buff[0] = '\0';

	return bytes;
}

/* Destroy the node n and return its next elem */
static inline struct Node *destroy_node(struct MessageQ *msg_q, struct Node *n) {
	struct Node *next = n->next;
	char *tmp;
	if (DEBUG) {
		/* If we are debugging then we want to show the freed payload so we
		 * need to put a null term on it */
		tmp = malloc(n->payload_size + 1);
		tmp[n->payload_size] = '\0';
		/* TODO: Add nullterm to this debug */
		/* debug("Freeing payload: %s\n", n->payload); */
	}
	msg_q->bytes -= n->payload_size;
	free(n->payload);
	free(n);

	return next;
}

/*********** Public API ***********/
/* Remove from msg_q->head through n (exclusive) */
static void prune(struct MessageQ *msg_q, struct Node *n) {
	struct Node *cur = msg_q->head;
	msg_q->head = n;

	while(n != (cur = destroy_node(msg_q, cur)));
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
static struct Node *append(struct MessageQ *msg_q, char *payload, long psize) {
	struct Node *empt, *new;

	if (!(empt = empty_node()))
		return NULL;

	/* Fill out the node that's currently empty (our new node) */
	msg_q->tail->payload = malloc(psize);
	memcpy(msg_q->tail->payload, payload, psize);
	msg_q->tail->payload_size = psize;
	msg_q->tail->next = empt;

	new = msg_q->tail;
	/* Now, move the tail to the new empty node */
	msg_q->tail = empt;

	/* Increase the byte count */
	msg_q->bytes += psize;

	return new;
}

/* Append another list onto the end of this one. */
static void append_list(struct MessageQ *msg_q, struct MessageQ *append) {
	msg_q->tail = append->head;
	msg_q->bytes += append->bytes;
}

void destroy(struct MessageQ *msg_q) {
	/* Free all of the nodes */
	prune(msg_q, msg_q->tail);
	free(msg_q);
}

struct MessageQ *message_q_init() {
	struct MessageQ *msg_q;
	/* We use an empty node as the head so that way when we hand out references
	 * to the node the references will be correct whenever the node is
	 * populated */
	struct Node *empt;

	if(!(msg_q = malloc(sizeof (struct MessageQ))))
		return NULL;

	if (!(empt = empty_node())) {
		free(msg_q);
		return NULL;
	}

	msg_q->head = empt;
	msg_q->tail = empt;
	msg_q->bytes = 0;

	/* Methods */
	msg_q->assemble = assemble;
	msg_q->prune  = prune;
	msg_q->append = append;
	msg_q->append_list = append_list;
	msg_q->destroy = destroy;

	return msg_q;
}
