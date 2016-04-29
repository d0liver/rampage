#include <stdlib.h>
#include <libwebsockets.h>
#include "channel.h"
#include "buff.h"
#define lwss libwebsocket

/* Get a handle for the channel. A handle is basically a reference that a user
 * can use to track what's going on in the channel. Since every user is likely
 * to be at a different place in terms of data that they need to fetch, we can
 * use this handle to keep track of which user we're dealing with. Note that
 * "user" here, just means some entity that's interested in the channel. It
 * could be something like an automated resource (notifications) and not a real
 * person. */
static struct ChannelHandle *handle(struct Channel *ch) {
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
	handle->channel = ch;

	return OK;
}

/* Assemble a message from the queue so that we can try to send it */
static struct Buff *assemble(struct ChannelHandle *handle, int *len) {
	struct LinkedList *lst = handle->channel->lst;
	struct Node *n = handle->head;
	char *buffp;

	if(!(buff = malloc(lst->bytes + LWS_SEND_BUFFER_PRE_PADDING)))
		return NULL;

	buff += LWS_SEND_BUFFER_PRE_PADDING;
	buffp = buff;

	while ((n = n->next) != lst->tail)
		buffp = memcpy(buffp, n->payload, n->payload_size) + n->payload_size;

	if (len)
		*len = buffp - buff;

	return buff;
}

static void check_free(struct Channel *ch) {
	struct Node *n;
	int i;

	if (!ch->num_handles)
		return;

    /*
	 * Start from the top of the list and iterate until we find the first
	 * handle (actually the handle's head). Since data is appended on the end
	 * everything before this handle will be unused now so it can be destroyed.
     */
	for (n = lst->head; ; n = n->next;)
		for (i = 0; i < ch->num_handles; ++i)
			if (n == ch->handles[i]->head)
				break 2;

	/* Remove all of the old data that has been ready by everybody already. */
	ch->lst->prune(n);
}

/* Assemble and flush messages to the user. */
static enum RmpgErr flush(struct ChannelHandle *handle, struct lwss *wsi) {
	struct LinkedList *lst = handle->channel->lst;
	char *buff;
	int len, bytes_written;

	/* TODO: It's actually not necessary to assemble here. We can just call
	 * write multiple times with whatever we have available */
	if(!(buff = assemble(handle, &len))
		return OUT_OF_MEM;

	bytes_written = lwss_write (wsi, buff, lst->bytes(), LWS_WRITE_TEXT);

	if (bytes_written < 0)
		return ERROR_WRITING_TO_SOCKET;

	/* FIXME: Maybe we should try again? */
	if (bytes_written < lst->bytes())
		return ERROR_PARTIAL_WRITE;

    /*
	 * The write was successful, we can go ahead and update our head
	 * (indicating that we no longer need those nodes)
     */
	handle->head = handle->channel->tail;

    /*
	 * There may be nodes that can be freed now (if we were the last one in the
	 * channel to use the nodes that we just released)
     */
	check_free(handle->channel);
	free(buff);

	return bytes_written;
}

/* Add the payload to the queue to be sent later */
enum RmpgErr send(struct ChannelHandle *handle, char *payload, long psize) {
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
