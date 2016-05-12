#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <string.h>

#include "event_mgr.h"
#include "err.h"

void hello(char *payload) {
}

struct EvtMap {
	char *type;
	void (*handle)(char *payload);
};

static struct EvtMap evt_maps[] = {
	{.type = "hello", .handle = hello},
};

/*
 * Handle a raw incoming message.
 */
enum RmpgErr evt_mgr_on_raw(char *buff, size_t len) {
	int i;
	json_t *root, *event;
	json_error_t err;
	char *payload, *type;

	if(!(root = json_loads(buff, 0, &err)))
		return ERROR_JSON_PARSE;

	if (json_unpack(
		root, "{s: {s: s}, s:s}",
		"event",
			"type", &type,
		"payload", &buff
	))
		return ERROR_JSON_PARSE;

	/* Call each event handler registered for the event. */
	for (i = 0; i < sizeof(evt_maps)/sizeof(evt_maps[0]); ++i)
		if (!strcmp(evt_maps[i].type, type))
			evt_maps[i].handle(buff);
}

/* Register an event handler for an event fired with type evt_type */
/* void evt_mgr_register( */
/* 	char *evt_type, */
/* 	void (*handle)(char *payload) */
/* ) { */
/* } */
