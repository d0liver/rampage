#include <stdlib.h>
#include <libwebsockets.h>
#include "channel.h"
#define lwss libwebsocket

/* Get a handle for the channel. A handle is basically a reference that a user
 * can use to track what's going on in the channel. Since every user is likely
 * to be at a different place in terms of data that they need to fetch, we can
 * use this handle to keep track of which user we're dealing with. Note that
 * "user" here, just means some entity that's interested in the channel. It
 * could be something like an automated resource (notifications) and not a real
 * person. */
static struct ChannelHandle *handle(struct Channel *ch, struct ) {
	if(!(*handle = malloc(sizeof struct ChannelHandle)))
		return OUT_OF_MEM;

	/*
	 * We don't bother with reading any history out to the user when the handle
	 * is created. Most of the history has already probably been destroyed.
	 * Reading out that info should be handled elsewhere, maybe with a database
	 * and a private channel to the user. i.o.w. a channel is not on its own
	 * enough to implement a chat room with history from before joining.
	 */
	handle->head = ch->lst->tail;

	return OK;
}

/* Assemble a message from the queue so that we can try to send it */
static enum RmpgErr assemble(struct ChannelHandle *handle, char **buff) {
	struct LinkedList *lst = handle->channel->lst;
	struct Node *n;
	char *buff, *buffpos;

	if(!(*buff = malloc(lst->bytes() + LWS_SEND_BUFFER_PRE_PADDING)))
		return OUT_OF_MEM;

	*buff += LWS_SEND_BUFFER_PRE_PADDING;
	buffpos = *buff;
	n = handle->tail;
	while (n = lst->next(n))
		buffpos = memcpy(buffpos, n->payload, n->payload_size);

	return OK;
}

static void check_free(struct Channel *ch) {
	struct LinkedList *lst = ch->lst;
	struct Node *n;
	struct ChannelHandle *back_handle;
	int i;

	if (!ch->num_handles)
		return;

	/* Iterate the handle tails and find the one in the back */
	for (n = ch->handles[0]; n = lst->next(n);)
		for (i = 0; i < ch->num_handles; ++i)
			if (n == ch->handles[i]->tail)
				back_handle = ch->handles[i];

	/* Remove the range from the end of the list till the tail of the
	 * back_handle */
	lst->remove_till(back_handle->tail);
}

/* Flush the data for the given handle to the user */
static enum RmpgErr flush(struct ChannelHandle *handle, struct lwss *wsi) {
	struct LinkedList *lst = handle->channel->lst;
	char *buff;
	int bytes_written;

	if(assemble(handle, &buff, &len))
		return OUT_OF_MEM;

	bytes_written = lwss_write (wsi, buff, lst->bytes(), LWS_WRITE_TEXT);
	free(buff);

	if (bytes_written < 0)
		return ERROR_WRITING_TO_SOCKET;

	if (bytes_written < lst->bytes())
		return ERROR_PARTIAL_WRITE;
	/* FIXME: Maybe we should try again? */

	/* The write was successful, we can go ahead and update our tail
	 * (indicating that we no longer need those nodes) */
	handle->tail = handle->channel->head;
	/* There may be nodes that can be freed now (if we were the last one in the
	 * channel to use the nodes that we just released) */
	check_free(handle->channel);

	return OK;
}

/* Add the payload to the queue to be sent later */
enum RmpgErr *send(struct ChannelHandle *handle, char *payload, long psize) {
	/* The node to be added */
	struct Node *n;
	struct LinkedList *lst = handle->channel->lst;

	if(lst->insert_new(lst, payload, psize))
		return OUT_OF_MEM;

	return OK;
}

/*
 * Communication between connected users flows through channels. Each user has
 * a list of channels that they receive events from and can publish events to.
 */
enum RmpgErr init_channel(struct Channel **ch) {
	struct LinkedList *lst;

	if(!(*ch = malloc(sizeof struct Channel)))
		return OUT_OF_MEM;

	/* Data */
	ch->name = NULL;
	if(linked_list_init(&lst)) {
		free(*ch);
		return OUT_OF_MEM;
	}
	ch->lst = lst;

	/* Methods */
	ch->handle = handle;
	ch->flush = flush;
	ch->send = send;

	return OK;
}
