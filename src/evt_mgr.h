#ifndef RMPG_EVENT_MGR_H

#include "channel.h"
#include "session.h"

void evt_mgr_connected(struct Session *sess);
enum RmpgErr evt_mgr_init(void);
enum RmpgErr rmpg_evt_mgr_on(
	const char *evt,
	void (*handle)(struct Session *, const char *, void *),
	void *cb_data
);
enum RmpgErr evt_mgr_receive(struct Session *sess, char *buff, size_t len);
enum RmpgErr rmpg_evt_mgr_emit(
	const char *evt, const char *buff,
	struct ChannelHandle **handles, int num_handles
);
void rmpg_evt_mgr_on_default(
	void (*)(const char *, struct Session *, const char *, void *),
	void *
);
#define RMPG_EVENT_MGR_H
#endif
