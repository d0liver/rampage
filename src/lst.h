#ifndef LST_H

struct List {
	unsigned int grow_by, num_elems;
	void **items;
};

struct List *lst_init(int grow_by);
int lst_append(struct List *lst, void *elem, int cp_size);

#define LST_H
#endif
