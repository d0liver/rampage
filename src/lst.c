/*
 * This is a very simple list implementation for general use with small,
 * generic things.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lst.h"

struct List *lst_init(int grow_by) {
	struct List *lst;

	if (!(lst = malloc(sizeof(struct List))))
		return NULL;

	lst->num_elems = 0;
	lst->grow_by = grow_by;

	return lst;
}

int lst_append(struct List *lst, void *elem, int cp_size) {
	int size = (lst->num_elems + (lst->grow_by - 1)) & ~(lst->grow_by - 1);

    /*
	 * We will make a copy and put that into the list if we were requested to
	 * do so.
     */
	if (cp_size) {
		void *tmp;
		if(!(tmp = malloc(cp_size)))
			return 0;

		memcpy(tmp, elem, cp_size);
	}

	/* Are we full? */
	if (size == lst->num_elems) {
		size += lst->grow_by;
		if (
			!lst->num_elems && !(lst->items = malloc(size)) ||
			lst->num_elems && !(lst->items = realloc(lst->items, size))
		)
			goto bail;
	}

	/* Add the new elem */
	lst->items[lst->num_elems++] = elem;
	return 1;

bail:
	if(cp_size)
		free(elem);
	return 0;
}
