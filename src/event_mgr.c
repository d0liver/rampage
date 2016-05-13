#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <string.h>

#include "lst.h"
#include "event_mgr.h"
#include "err.h"

struct EvtMap {
	const char *evt;
	void (*handle)(const char *);
};

static struct List *evt_maps;

enum RmpgErr evt_mgr_init(void) {
	if(!(evt_maps = lst_init(/* Grow by */ 4)))
		return OUT_OF_MEM;

	return OK;
}

enum RmpgErr evt_mgr_on(const char *evt, void (*handle)(const char *)) {
	struct EvtMap *map;

	if (!(map = malloc(sizeof(struct EvtMap))))
		return OUT_OF_MEM;

	map->evt = evt;
	map->handle = handle;

	lst_append(evt_maps, map);

	return OK;
}

/*
 * Translate an incoming message into appropriate emits.
 */
enum RmpgErr evt_mgr_receive(char *buff, size_t len) {
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
	for (i = 0; i < evt_maps->num_elems; ++i) {
		struct EvtMap *map = (struct EvtMap *)evt_maps->items[i];

		if (!strcmp(map->evt, type))
			map->handle(buff);
	}
}
