#ifndef RMPG_CHANNEL_H

#include <libwebsockets.h>

#include "message_q.h"

#define MAX_CHANNEL_HANDLES 1000

/* See "channel_init" */
struct Channel {
	char *name;
	struct MessageQ *msg_q;

    /*
	 * We keep a list of the handles that we have issued so that we know when
	 * to clean things up.
     */
	struct ChannelHandle *handles[MAX_CHANNEL_HANDLES];
	int num_handles;

	/* Methods */
	struct ChannelHandle *(* handle)(struct Channel *);
	enum RmpgErr (* flush)(struct ChannelHandle *, struct lws *wsi);
	enum RmpgErr (* snd)(struct ChannelHandle *, char *, long);
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

struct Channel *channel_init(void);

#define RMPG_CHANNEL_H
#endif
