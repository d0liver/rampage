#include "linked_list.h"

#ifndef CHANNEL_H

/* Communication between connected users flows through channels. Each user has
 * a list of channels that they receive events from and can publish events to. */
struct Channel {
	char *name;
	struct LinkedList *lst;

	/* We keep a list of the handles that we have issued so that we know when
	 * to clean things up */
	struct ChannelHandle *handles;
	int num_handles;

	/* Methods */
	enum RmpgErr (* handle)(struct Channel *, struct ChannelHandle **);
	enum RmpgErr (* flush)(struct ChannelHandle *, char *);
	enum RmpgErr (* send)(struct ChannelHandle *, char *, long);
};

/* We hand out one of these to anyone who is interested in communicating over
 * this channel. This is what they hand back to use to perform operations on
 * the channel. */
struct ChannelHandle {
	struct Channel *channel;
	/* Everything after this up to the list head still needs to be sent */
	struct Node *tail;
};

enum RmpgErr init_channel(struct Channel **ch);

#define CHANNEL_H
#endif
