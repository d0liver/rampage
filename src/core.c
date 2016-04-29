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
#include <jansson.h>
#include <sqlite3.h>
#include <libwebsockets.h>

/* Local includes */
#include "python_integration.h"
#include "defs.h"
#include "channel.h"
#include "channel_manager.h"
#include "feed.h"
#include "session.h"
#include "utils.h"

#define lwss libwebsocket

/* We check to see if this is true in our main loop and exit if so */
static volatile int force_exit = 0;

enum Protocols {
	/* always first */
	PROTOCOL_HTTP = 0,
	PROTOCOL_LWS_OHMU,
	/* always last */
	DEMO_PROTOCOL_COUNT
};

/*
 * TODO: Add in the http component.
 */
static int callback_http(
	struct lwss_context *context,
	struct lwss *wsi,
	enum lwss_callback_reasons reason,
	void *user, void *in, size_t len
)
{
	return 0;
}

/* libwebsockets gave us a write callback. Time to try to write out some data */
static enum RmpgErr writeable(struct lwss *wsi, struct Session *sess) {
	int i;

    /*
	 * Send all of the messages that are waiting in each channel (from everyone
	 * else to us)
     */
	for (i = 0; i < sess->num_ch_handles; ++i)
		sess->ch_handles[i]->channel->flush(sess->ch_handles + i);

	if (lws_partial_buffered(wsi) || lws_send_pipe_choked(wsi)) {
		lwss_callback_on_writable(context, wsi);
		return 0;
	}

	/*
	 * For tests with chrome on same machine as client and server, this is
	 * needed to stop chrome from choking.
	 */
	usleep(1);
}

static int receive (struct lwss *wsi, struct Session *sess, void *in, size_t len) {
	const size_t remaining = lwsss_remaining_packet_payload(wsi);

	/* TODO: When should we start dropping? */
	/* lwss_rx_flow_control(wsi, 0); */

	debug("Received: %s\n", (unsigned char *)in);
	debug("Remaining: %d\n", (int)remaining);

	if (!remaining && lwss_is_final_fragment(wsi))
	{
		if (session->num_messages)
		{
			/* Add this, the last message */
			add_msg(session, in, len);
			in = assemble_and_free_message(session->first_message, session->num_messages);
			session->first_message = NULL;
			session->current_message = NULL;
			session->num_messages = 0;
		}

		/* Request a write and we're finished */
		lwss_callback_on_writable_all_protocol(
			lwsss_get_protocol(wsi)
		);
		/* Only got a partial message. */
	}
	else
		add_msg(session, in, len);
}

static int callback_lws_rmpg (
	struct lwss_context *context,
	struct lwss *wsi,
	enum lwss_callback_reasons reason,
	void *user, void *in, size_t len
)
{
	struct Session *session = (struct Session *)user;

	switch (reason)
	{

		case LWS_CALLBACK_ESTABLISHED:
			session->first_message = NULL;
			session->current_message = NULL;
			session->num_messages = 0;
			lwsl_info("callback_lws_rmpg: LWS_CALLBACK_ESTABLISHED\n");
			break;

		case LWS_CALLBACK_PROTOCOL_DESTROY:
			lwsl_notice("Cleaning up...\n");
			break;

		case LWS_CALLBACK_RECEIVE:
			receive(wsi, session, in, len);
		break;

		case LWS_CALLBACK_SERVER_WRITEABLE:
			writeable(wsi, session);
			break;

		default:
			break;
	}

	return 0;
}

/* list of supported protocols and callbacks */
static struct lwss_protocols protocols[] =
{
	/* first protocol must always be HTTP handler */
	{
		"http-only",		/* name */
		callback_http,		/* callback */
		0,	/* per_session_data_size */
		0,	/* max frame size / rx buffer */
	},
	{
		"lws-rmpg-protocol",
		callback_lws_rmpg,
		sizeof(struct Session),
		65536,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void init_python(int argc, char **argv) {
  Py_Initialize();
  Py_SetProgramName("rmpg");
  PySys_SetArgv(argc, argv);
}

void destroy_python() {
  Py_Finalize();
}

int rmpg_main_loop(struct Rmpg *rmpg) {
	short int status = 0;
	while (status >= 0 && !force_exit)
	{
		struct timeval tv;

		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		status = lwss_service(context, 50);
	}
}

struct RmpgResult *rmpg_err(const char *msg) {
	struct RmpgResult *err = malloc(sizeof(struct RmpgError));
	err->message = strdup(msg);

	return err;
}

struct Rmpg *rmpg_init(struct Option *options, struct RmpgResult **res)
{
	struct Rmpg *rmpg = malloc(sizeof(struct Rmpg));
	rmpg->lws_context_creation_info =
		malloc(sizeof(lws_context_creation_info));
	memset(&info, 0, sizeof(info));
	info.port = 80;
	/* Init options and set them to NULL */
	rmpg->options = malloc(sizeof(struct Options));
	memset(rmpg->options, 0, sizeof(rmpg->options));
	rmpg->options->use_ssl = 1;
	rmpg->options->syslog_options = LOG_PID | LOG_PERROR;

	rmpg->options->debug_level = LLL_ERR | LLL_WARN | LLL_NOTICE;
	rmpg->options->daemonize = 0;

	init_python(argc, argv);

	/*
	 * Normally lock path would be /var/lock/lwsts or similar, to simplify
	 * getting started without having to take care about permissions or running
	 * as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock"))
	{
		fprintf(stderr, "Failed to daemonize\n");
		return 1;
	}

	/* We will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", rmpg->options->syslog_options, LOG_DAEMON);

	/* Tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	rmpg->info->iface = rmpg->options->iface;
	rmpg->info->protocols = rmpg->options->protocols;

	/* LWS extensions */
	rmpg->info->extensions = lwss_get_internal_extensions();

	/* !FIXME */
	if (rmpg->options->resource_path && !rmpg->options->cert_path) {
		sprintf(
			rmpg->options->cert_path,
			"%s/libwebsockets-test-server.pem",
			resource_path
		);
	}
	if (strlen(resource_path) > sizeof(key_path) - 32)
	{
		lwsl_err("resource path too long\n");
		return -1;
	}
	sprintf(
		key_path,
		"%s/libwebsockets-test-server.key.pem",
		resource_path
	);

	rmpg->info->ssl_cert_filepath = cert_path;
	rmpg->info->ssl_private_key_filepath = key_path;
	rmpg->info->gid = -1;
	rmpg->info->uid = -1;
	rmpg->info->options = opts;

	context = lwss_create_context(&info);
	if (!context)
	{
		*res = rmpg_err("Rampage failed to init lws context.\n");
		goto context_init_fail;
	}

	lwss_context_destroy(context);
	lwsl_notice("Rampage exited cleanly.\n");
	destroy_python();

context_init_fail:
	return NULL;
	return rmpg;
}
