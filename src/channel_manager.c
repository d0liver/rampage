#include <stdlib.h>
#include "channel_manager.h"
#include "channel.h"

struct ChannelMgr *init_channel_manager(void) {
	struct ChannelMgr *mgr;

	if (!(mgr = malloc(sizeof struct ChannelMgr)))
		return NULL;

	/* Nothing special about "world" except that each session will take out a
	 * handle on it when it's initialized */
	mgr->world = init_channel();
}
