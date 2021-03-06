#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#include <libwebsockets.h>

/* Local includes */
#include "read_ini.h"
#include "err.h"
#include "http.h"
#include "evt_mgr.h"
#include "rampage.h"
#include "defs.h"
#include "channel.h"
#include "session.h"

void (*user_init_handler)(struct Session *);
static struct RampageOptions {
	char cert_path[1024], key_path[1024], interface_name[128];
	int n, use_ssl, opts;
	const char *iface;
	int syslog_options;
	int debug_level;
	int daemonize;
} rmpg_opts = {
	.interface_name = "",
	.debug_level = 7,
	.syslog_options = LOG_PID | LOG_PERROR
};
static int callback_lws_rmpg(struct lws *, enum lws_callback_reasons, void *, void *, size_t);
static struct Channel *world;

static struct lws_context *context;
static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",		/* name */
		http_callback,		/* callback */
		65536,	/* per_session_data_size */
		65536,	/* max frame size / rx buffer */
	},
	{
		"lws-rmpg-protocol",
		callback_lws_rmpg,
		sizeof(struct Session),
		65536,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};
/* We check to see if this is true in our main loop and exit if so */
static volatile int force_exit = 0;
static struct Channel *world;
static struct lws_context_creation_info info = {.port = 7681};
static char *resource_path;
static struct option long_opts[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "allow-non-ssl",	no_argument,		NULL, 'a' },
	{ "interface",  required_argument,	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },
	{ "libev",  no_argument,		NULL, 'e' },
	{ "daemonize", 	no_argument,		NULL, 'D' },
	{ "resource_path", required_argument,		NULL, 'r' },
	{ NULL, 0, 0, 0 }
};
static const char *short_opts = "eci:hsap:d:Dr:";

/* This is the public api for registering an event handler with rampage */
enum RmpgErr rmpg_on(
	const char *evt,
	void (*handle)(struct Session *sess, const char *),
	void *cb_data
) {
	rmpg_evt_mgr_on(evt, cb_data, handle);

	return OK;
}

/* lwss gave us a write callback. Time to try to write out some data */
static int writeable(struct lws *wsi, struct Session *sess) {
	int i;

    /*
	 * Send all of the messages that are waiting in each channel (from everyone
	 * else to us)
     */
	for (i = 0; i < sess->ch_handles->num_elems; ++i) {
		struct ChannelHandle *h = sess->ch_handles->items[i];
		if(h->channel->flush(h, wsi) == ERROR_OUT_OF_MEMORY)
			goto out_of_memory;
	}

	if (lws_partial_buffered(wsi) || lws_send_pipe_choked(wsi)) {
		lws_callback_on_writable(wsi);
		return 0;
	}

	/*
	 * For tests with chrome on same machine as client and server, this is
	 * needed to stop chrome from choking.
	 */
	usleep(1);

	return 0;

out_of_memory:
	fprintf(stderr, "Out of memory.\n");
	return 0;
}

static int receive (struct lws *wsi, struct Session *sess, void *in, size_t len) {
	const size_t remaining = lws_remaining_packet_payload(wsi);

	/* TODO: When should we start dropping? */
	/* lwss_rx_flow_control(wsi, 0); */

	/* Message is finished. */
	if (!remaining && lws_is_final_fragment(wsi))
	{
		char *buff;

		/* Allocate a buffer to assemble the full message into */
		if(!(buff = malloc(sess->pending->bytes + len + 1)))
			goto out_of_mem;

		if(!sess->pending->append(sess->pending, in, len))
			goto out_of_mem;

		/* Assemble the whole message */
		sess->pending->assemble(
			sess->pending,
			sess->pending->head,
			buff
		);

        /*
		 * Do useful things! We have a message now and we can do things with it
		 * (like send it to a channel for other users to look at).
         */
		if(evt_mgr_receive(sess, buff, len) == ERROR_JSON_PARSE)
			goto json_parse_fail;

		lws_callback_on_writable(wsi);

        /*
		 * This will also free up the previous payloads attached to the list.
		 * NOTE: This _must_ come last because pruning also reduces the byte
		 * count which we use before this.
         */
		sess->pending->prune(sess->pending, sess->pending->tail);
	}
	/* Only got a partial message. */
	else if(!sess->pending->append(sess->pending, in, len))
		goto out_of_mem;

	return 0;

out_of_mem:
	fprintf(stderr, "Out of memory.\n");
	return 0;

json_parse_fail:
	fprintf(stderr, "Failed to parse incoming json request.\n");
	return 0;
}

static void init(struct lws *wsi, struct Session *sess, void *in, size_t len) {
	if(session_init(sess) == ERROR_OUT_OF_MEMORY)
		fprintf(stderr, "Out of memory.\n");
	
	user_init_handler(sess);
}

static int callback_lws_rmpg (
	struct lws *wsi,
	enum lws_callback_reasons reason,
	void *session, void *in, size_t len
)
{
	struct Session *sess = (struct Session *)session;
	struct Channel *world;

	switch (reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
			init(wsi, session, in, len);
			evt_mgr_connected(sess);
			break;

		case LWS_CALLBACK_PROTOCOL_DESTROY:
			fprintf(stderr, "Cleaning up.\n");
			break;

		case LWS_CALLBACK_RECEIVE:
			receive(wsi, session, in, len);
		break;

		case LWS_CALLBACK_SERVER_WRITEABLE:
			writeable(wsi, session);
			break;

		/* TODO: Add in cleanup for socket closing */
	}

	return 0;
}

void init_context(void) {
	debug("Build context...\n");
	/* We will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", rmpg_opts.syslog_options, LOG_DAEMON);

	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(rmpg_opts.debug_level, lwsl_emit_syslog);

	/* TODO: Include license/copyright info. */
	debug("Rampage websockets server..\n");

	info.iface = rmpg_opts.iface;
	info.protocols = protocols;
	info.extensions = exts;
	if (!rmpg_opts.use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	}
	else {
		if (strlen(resource_path) > sizeof(rmpg_opts.cert_path) - 32)
			lwsl_err("resource path too long\n");
		sprintf(rmpg_opts.cert_path, "%s/lwss-test-server.pem",
				resource_path);
		if (strlen(resource_path) > sizeof(rmpg_opts.key_path) - 32)
			lwsl_err("resource path too long\n");
		sprintf(rmpg_opts.key_path, "%s/lwss-test-server.key.pem",
				resource_path);

		info.ssl_cert_filepath = rmpg_opts.cert_path;
		info.ssl_private_key_filepath = rmpg_opts.key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = rmpg_opts.opts;

	context = lws_create_context(&info);
	if (context == NULL)
		lwsl_err("lws init failed\n");
}

/* Handle the config options from rampage.ini */
void config_handler(struct Option *opt) {

	/*
	 * If opt is NULL then we are finished parsing options so we can build the
	 * context from it.
	 */
	if (!opt) {
		init_context(/* uses rmpg_opts */);
		return;
	}
	if (!strcmp(opt->key, "libev"))
		rmpg_opts.opts |= LWS_SERVER_OPTION_LIBEV;
	else if(!strcmp(opt->key, "daemonize"))
		rmpg_opts.daemonize = 1;
	else if (!strcmp(opt->key, "debug_level"))
		rmpg_opts.debug_level = atoi(opt->value);
	else if (!strcmp(opt->key, "use_ssl"))
		rmpg_opts.use_ssl = 1;
	else if (!strcmp(opt->key, "allow_ssl_on_non_ssl_port"))
		rmpg_opts.opts |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
	else if (!strcmp(opt->key, "port"))
		info.port = atoi(opt->value);
	else if (!strcmp(opt->key, "interface_name")) {
		strncpy(rmpg_opts.interface_name, opt->value, sizeof(rmpg_opts.interface_name));
		/* In case of truncation. */
		rmpg_opts.interface_name[sizeof(rmpg_opts.interface_name) - 1] = '\0';
		rmpg_opts.iface = rmpg_opts.interface_name;
	}
	else if (!strcmp(opt->key, "resource_path")) {
		resource_path = opt->value;
		printf("Setting resource path to \"%s\"\n", resource_path);
	}
}

void rmpg_register_init_handler(void (*handler)(struct Session *)) {
	user_init_handler = handler;
}

void rmpg_init(void (*user_config_handler)(struct Option *)) {
	/* First, read the config */
	FILE *fp = fopen("rampage.ini", "rb");
	void (*handle_opt[])(struct Option *) =
		{config_handler, user_config_handler, NULL};

	if (!fp) {
		/* FIXME: Error handling */
		debug("An error occurred opening the config file.\n");
		return;
	}
	read_config(fp, handle_opt);

	evt_mgr_init();
	http_init();

	/* FIXME: What's the best recovery option here? */
	if(!(world = channel_init())) {
		fprintf(stderr, "Couldn't initialize world channel\n");
		exit(1);
	}
}

void rmpg_loop(void) {
	int status = 0;

    /*
	 * Do the central poll loop.
	 * Negative statuses from lwss indicate an error.
     */
	while (status >= 0 && !force_exit)
		status = lws_service(context, /* Timeout */ 50);
}

void rmpg_cleanup(void) {
	/* Cleanup */
	lws_context_destroy(context);
	debug("Rampage exited cleanly\n");
}
