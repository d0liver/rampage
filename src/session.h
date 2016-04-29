#ifndef SESSION_H

/* A user's session */
struct Session {
	struct lwss *wsi;
	/* The channels that this session is subscribed to */
	struct Channel *channels;
};

#define SESSION_H
#endif
