#ifndef SESSION_H

#include "lst.h"
#include "channel.h"

/* A user's session */
struct Session {
	struct lwss *wsi;
	/* The channels that this session is subscribed to */
	struct List *ch_handles;
	struct MessageQ *pending;
};

enum RmpgErr session_init(struct Session *sess);
void session_destroy(struct Session *sess);
enum RmpgErr session_subscribe(struct Session *sess, struct Channel *world);

#define SESSION_H
#endif
