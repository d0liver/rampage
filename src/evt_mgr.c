#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <string.h>

#include "lst.h"
#include "channel.h"
#include "evt_mgr.h"
#include "err.h"

void (*default_handler)(const char *, struct Session *, const char *, void *);
void *default_cb_data;

struct EvtMap {
	const char *evt;
	void *cb_data;
	void (*handle)(struct Session *, const char *, void *);
};

static struct List *evt_maps;

static void notify(struct Session *sess, const char *type, const char *buff) {
	int i;

	debug("Notifying event handlers for type %s\n", type);
	/* Call each event handler registered for the event. */
	for (i = 0; i < evt_maps->num_elems; ++i) {
		struct EvtMap *map = (struct EvtMap *)evt_maps->items[i];

		debug("Event map type: %s\n", map->evt);
		if (!strcmp(map->evt, type)) {
			debug("Firing event: %s, %s\n", map->evt, type);
			map->handle(sess, buff, map->cb_data);
			return;
		}
	}
	/*
	 * There was no handler for the event type. We will call the handler
	 * default_handler.
	 */
	default_handler(type, sess, buff, default_cb_data);
}

enum RmpgErr evt_mgr_init(void) {
	printf("Initialized event manager...\n");
	if(!(evt_maps = lst_init(/* Grow by */ 4)))
		return ERROR_OUT_OF_MEMORY;

	return OK;
}

enum RmpgErr rmpg_evt_mgr_emit(
	const char *evt, const char *buff,
	struct ChannelHandle **handles, int num_handles
) {
	json_t *msg;
	char *res;
	int i;

	if(!(msg = json_pack(
		"{s: {s: s}, s:s}",
		"event",
			"type", evt,
		"payload", buff
	)))
		return ERROR_JSON_PACK;
	res = json_dumps(msg, 0);

	debug("Emit Event: %s\n", evt);
	debug("Emit Payload: %s\n", buff);
	/* Send to all of the requested channels */
	for (i = 0; i < num_handles; ++i) {
		debug("Sending to handle...\n");
		handles[i]->channel->snd(handles[i], res, strlen(res));
	}

	return OK;
}

/* We have been informed that the session has connected */
void evt_mgr_connected(struct Session *sess) {
	notify(sess, "connected", NULL);
}

/* Set the handler that will be called when no other handler is found */
void rmpg_evt_mgr_on_default(
	void (*handler)(const char *, struct Session *, const char *, void *),
	void *cb_data
) {
	default_handler = handler;
	default_cb_data = cb_data;
}

enum RmpgErr rmpg_evt_mgr_on(
	const char *evt,
	void (*handle)(struct Session *, const char *, void *),
	void *cb_data
) {
	struct EvtMap *map;

	if (!(map = malloc(sizeof(struct EvtMap))))
		return ERROR_OUT_OF_MEMORY;

	map->evt = evt;
	map->handle = handle;
	map->cb_data = cb_data;

	debug("Registered event: %s\n", map->evt);
	lst_append(evt_maps, map, 0);

	return OK;
}

/*
 * Translate an incoming message into appropriate emits.
 */
enum RmpgErr evt_mgr_receive(struct Session *sess, char *buff, size_t len) {
	int i;
	json_t *root, *event;
	json_error_t err;
	char *payload, *type;

	debug("Event manager received.\n");

	if(!(root = json_loads(buff, 0, &err))) {
		debug("Failed to parse.\n");
		return ERROR_JSON_PARSE;
	}

	debug("Parsing: %s\n", buff);
	if (i = json_unpack_ex(
		root, &err, 0, "{s: {s: s}, s:s}",
		"event",
			"type", &type,
		"payload", &buff
	)) {
		debug("Failed to unpack: %s\n", err.text);
		return ERROR_JSON_PARSE;
	}

	notify(sess, type, buff);

	return OK;
}
