#ifndef LINKED_LIST_H

struct LinkedList {
	void (* assemble)(struct LinkedList *lst, struct Node *n, char *buff) {
	void (* prune)(struct LinkedList *, struct Node *);
	void (* append)(struct LinkedList *, char *, long);
	void (* append_list)(struct LinkedList *, struct LinkedList *);
	void (* destroy)(struct LinkedList *);

	/* Each entry in the linked list points to a buffer containing the message
	 * payload. If we want to limit the total amount that is stored within the
	 * particular list we can use this to track the current amount. */
	long bytes;
	struct Node *head, *tail;
};

struct Node {
	/* The number of references currently held to this node. Once this drops to
	 * zero we can remove it from the list and free it */
	long payload_size;

	struct Node *next;
	char *payload;
};

enum RmpgErr linked_list_init(struct LinkedList **lst);

#define LINKED_LIST_H
#endif
