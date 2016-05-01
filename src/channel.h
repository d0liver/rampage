#include "linked_list.h"

#ifndef CHANNEL_H

/* See "init_channel" */
struct Channel {
	char *name;
	struct LinkedList *lst;

    /*
	 * We keep a list of the handles that we have issued so that we know when
	 * to clean things up.
     */
	struct ChannelHandle *handles;
	int num_handles;

	/* Methods */
	struct ChannelHandle *(* handle)(struct Channel *, struct ChannelHandle **);
	enum RmpgErr (* flush)(struct ChannelHandle *, char *);
	enum RmpgErr (* send)(struct ChannelHandle *, char *, long);
};

/* See "handle" method */
struct ChannelHandle {
	/* The underlying channel that we're tracking */
	struct Channel *channel;

	/*
	 * Everything after this (but including this) up to the list tail still
	 * needs to be sent.
	 */
	struct Node *head;
};

struct Channel *init_channel(void);

#define CHANNEL_H
#endif
