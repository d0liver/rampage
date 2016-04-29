#include <channel.h>
#ifndef CHANNEL_MANAGER_C

/* Responsible for things like channel discovery, and other operations that
 * happen across channels */
struct ChannelManager {
	struct Channel **channels, *world;
};

#define CHANNEL_MANAGER_C
#endif
