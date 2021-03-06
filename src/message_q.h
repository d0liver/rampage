#ifndef RMPG_LINKED_LIST_H

#include "err.h"

struct Node {
	/* The number of references currently held to this node. Once this drops to
	 * zero we can remove it from the list and free it */
	long payload_size;

	struct Node *next;
	char *payload;
};

struct MessageQ {
	long (* assemble)(struct MessageQ *msg_q, struct Node *n, char *buff);
	void (* prune)(struct MessageQ *, struct Node *);
	struct Node *(* append)(struct MessageQ *, char *, long);
	void (* append_list)(struct MessageQ *, struct MessageQ *);
	void (* destroy)(struct MessageQ *);

	/* Each entry in the linked list points to a buffer containing the message
	 * payload. If we want to limit the total amount that is stored within the
	 * particular list we can use this to track the current amount. */
	long bytes;
	struct Node *head, *tail;
};

struct MessageQ *message_q_init();

#define RMPG_LINKED_LIST_H
#endif
