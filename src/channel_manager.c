#include <stdlib.h>

#include "channel_manager.h"

struct ChannelManager *init_channel_manager(void) {
	struct ChannelManager *mgr;

	if (!(mgr = malloc(sizeof (struct ChannelManager))))
		return NULL;

	/* Nothing special about "world" except that each session will take out a
	 * handle on it when it's initialized */
	mgr->world = init_channel();
}
