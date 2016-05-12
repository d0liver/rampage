#ifndef SESSION_H

/* A user's session */
struct Session {
	struct lwss *wsi;
	/* The channels that this session is subscribed to */
	struct ChannelHandle **ch_handles;
	int num_ch_handles;
	struct MessageQ *pending;
};

#define SESSION_H
#endif
