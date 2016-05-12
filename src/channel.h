#ifndef CHANNEL_H

#include <libwebsockets.h>

#include "lws_short.h"
#include "message_q.h"

/* See "init_channel" */
struct Channel {
	char *name;
	struct MessageQ *msg_q;

    /*
	 * We keep a list of the handles that we have issued so that we know when
	 * to clean things up.
     */
	struct ChannelHandle *handles;
	int num_handles;

	/* Methods */
	struct ChannelHandle *(* handle)(struct Channel *);
	enum RmpgErr (* flush)(struct ChannelHandle *, lws_wsi *wsi);
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

struct Channel *init_channel(void);

#define CHANNEL_H
#endif
