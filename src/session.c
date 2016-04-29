#include <stdlib.h>
#include <stdio.h>

#include "session.h"
#include "channel.h"

/* Initialize a session. Libwebsockets allocates the session for us. */
int init_session(struct lwss *wsi, struct Session *sess, void *in, size_t len) {
	struct Channel *world;

	/* TODO: Check for OOM */
	/* Initialize the world channel */
	sess->channel_manager = init_channel_manager();
	world = sess->channel_manager->world;
	sess->world = world->handle(world);

	sess->first_message = NULL;
	sess->current_message = NULL;
	sess->num_messages = 0;

	debug("Initialized session\n");

	return 0;
}
