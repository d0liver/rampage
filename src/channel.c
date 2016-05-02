#include <stdlib.h>

#include "channel.h"

/* Get a handle for the channel. A handle is basically a reference that a user
 * can use to track what's going on in the channel. Since every user is likely
 * to be at a different place in terms of data that they need to fetch, we can
 * use this handle to keep track of which user we're dealing with. Note that
 * "user" here, just means some entity that's interested in the channel. It
 * could be something like an automated resource (notifications) and not a real
 * person. */
static struct ChannelHandle *handle(struct Channel *ch) {
	struct ChannelHandle *handle;

	if(!(handle = malloc(sizeof (struct ChannelHandle))))
		return NULL;

	/*
	 * We don't bother with reading any history out to the user when the handle
	 * is created. Most of the history has already probably been destroyed.
	 * Reading out that info should be handled elsewhere, maybe with a database
	 * and a private channel to the user. i.o.w. a channel is not on its own
	 * enough to implement a chat room with history from before joining.
	 */
	handle->head = ch->lst->tail;
	handle->channel = ch;

	return handle;
}

/* Assemble a message from the queue so that we can try to send it */
static void check_free(struct Channel *ch) {
	struct LinkedList *lst = ch->lst;
	struct Node *n;
	int i;

	if (!ch->num_handles)
		return;

    /*
	 * Start from the top of the list and iterate until we find the first
	 * handle (actually the handle's head). Since data is appended on the end
	 * everything before this handle will be unused now so it can be destroyed.
     */
	for (n = lst->head; ; n = n->next)
		for (i = 0; i < ch->num_handles; ++i)
			if (n == ch->handles[i].head)
				goto done;
done:

	/* Remove all of the old data that has been ready by everybody already. */
	ch->lst->prune(ch->lst, n);
}

/* Assemble and flush messages to the user. */
enum RmpgErr flush(struct ChannelHandle *handle, lws_wsi *wsi) {
	struct LinkedList *lst = handle->channel->lst;
	int len, bytes_written;
	char *buff;

	if(!(buff = malloc(
		lst->bytes +
		LWS_SEND_BUFFER_PRE_PADDING +
		LWS_SEND_BUFFER_POST_PADDING
	)))
		return OUT_OF_MEM;

	buff += LWS_SEND_BUFFER_PRE_PADDING;

    /*
	 * TODO: It's actually not necessary to assemble here. We can just call
	 * write multiple times with whatever we have available. However, if we do
	 * things that way then we will need to move all of the data anyway because
	 * it needs to be preceded with some padding for lws (this seems like a
	 * shitty design). Maybe ideas in the future will be better? It would be
	 * nice, maybe, if we didn't need to move this data to send it.
     */
	lst->assemble(lst, handle->head, buff);

	bytes_written = lwss_write (wsi, buff, lst->bytes, LWS_WRITE_TEXT);

	if (bytes_written < 0)
		return ERROR_WRITING_TO_SOCKET;
	/* FIXME: Maybe we should try again? */
	else if (bytes_written < lst->bytes)
		return ERROR_PARTIAL_WRITE;

    /*
	 * The write was successful, we can go ahead and update our head
	 * (indicating that we no longer need those nodes)
     */
	handle->head = handle->channel->lst->tail;

    /*
	 * There may be nodes that can be freed now (if we were the last one in the
	 * channel to use the nodes that we just released)
     */
	check_free(handle->channel);
	free(buff);

	return bytes_written;
}

/* Add the payload to the queue to be sent later */
static enum RmpgErr snd(struct ChannelHandle *handle, char *payload, long psize) {
	/* The node to be added */
	struct Node *n;
	struct LinkedList *lst = handle->channel->lst;

	if(lst->append(lst, payload, psize))
		return OUT_OF_MEM;

	return OK;
}

/*
 * TODO: Implement send_nodes (add nodes directly to the queue to keep from
 * having to copy messages that won't be parsed before sending.)
 */

/*
 * Communication between connected users flows through channels. Each user has
 * a list of channels that they receive events from and can publish events to.
 */
struct Channel *init_channel(void) {
	struct LinkedList *lst;
	struct Channel *ch;

	if(!(ch = malloc(sizeof (struct Channel))))
		return NULL;

	/* Data */
	ch->name = NULL;
	if(linked_list_init(&lst)) {
		free(ch);
		return NULL;
	}
	ch->lst = lst;

	/* Methods */
	ch->handle = handle;
	ch->flush = flush;
	ch->snd = snd;

	return ch;
}
