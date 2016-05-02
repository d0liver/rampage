#ifndef CHANNEL_MANAGER_H

#include "channel.h"

/*
 * Responsible for things like channel discovery, and other operations that
 * happen across channels.
 */
struct ChannelManager {
	struct Channel *channels, *world;
};

struct ChannelManager *init_channel_manager(void);

#define CHANNEL_MANAGER_H
#endif
