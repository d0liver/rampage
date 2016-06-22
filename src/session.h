#ifndef RMPG_SESSION_H

#include "lst.h"
#include "channel.h"

/* A user's session */
struct Session {
	/* The channels that this session is subscribed to */
	struct List *ch_handles;
	struct MessageQ *pending;
	/* This library user can store data here */
	void *data;
};

enum RmpgErr session_init(struct Session *sess);
void session_destroy(struct Session *sess);
void rmpg_register_init_handler(void (*)(struct Session *));

#define RMPG_SESSION_H
#endif
