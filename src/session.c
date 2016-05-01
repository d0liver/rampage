#include <stdlib.h>
#include <stdio.h>

#include "session.h"
#include "channel.h"

/* Initialize a session. Libwebsockets allocates the session for us. */
int init_session(
	struct Session *sess,
	struct lwss *wsi,
	void *in, size_t len
) {
	if(!(sess->pending = linked_list_init()))
		return -1;

	debug("Initialized session\n");

	return 0;
}

void destroy_session(struct Session *sess) {
	sess->pending->destroy(sess->pending);
}
