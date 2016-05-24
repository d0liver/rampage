#include <stdlib.h>
#include <stdio.h>

#include "session.h"
#include "channel.h"

/* Initialize a session. Libwebsockets allocates the session for us. */
enum RmpgErr session_init(struct Session *sess) {
	if(!(sess->pending = message_q_init()))
		return ERROR_OUT_OF_MEMORY;

	if (!(sess->ch_handles = lst_init(4))) {
		sess->pending->destroy(sess->pending);
		return ERROR_OUT_OF_MEMORY;
	}

	debug("Initialized session\n");

	return 0;
}

enum RmpgErr rmpg_session_subscribe(struct Session *sess, struct Channel *world) {
	struct ChannelHandle *hndl = world->handle(world);

	debug("Subscribed session to channel.\n");
	if(!hndl)
		return ERROR_OUT_OF_MEMORY;

	lst_append(sess->ch_handles, hndl, 0);
}

void session_destroy(struct Session *sess) {
	sess->pending->destroy(sess->pending);
}
