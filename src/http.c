#include <string.h>
#include <stdio.h>
#include <libwebsockets.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errors.h"
#include "http.h"

#define MAX_REQUEST_URI_LEN
#define NUM_ROUTES_INC 16

static char *get_mimetype(const char *fname)
{
	int len = strlen(fname), i;
	struct Extension extensions[] = {
		{".ico", "image/x-icon"},
		{".png", "image/png"},
		{".html", "text/html"},
		{".css", "text/css"},
		{".js", "text/javascript"},
		{".ttf", "application/octet-stream"}
	};

	if (len < 5)
		return NULL;

	for (i = 0; i < sizeof(extensions)/sizeof(*extensions); ++i)
		if (!strcmp(&fname[len - strlen(extensions[i].file_ext)], extensions[i].file_ext)) {
			char *result = malloc(strlen(extensions[i].mimetype)+1);
			strcpy(result, extensions[i].mimetype);
			return result;
		}

	return NULL;
}

/* Check that the child path is actually in a subdirectory of the parent. We
 * use this to make sure that requested resources are actually underneath the
 * specified RESOURCE_PATH to prevent unauthorized access to resources */
static char *verify_path(const char *parent_path, const char *child_path) {
	char * real_parent_path, *real_child_path;
	real_parent_path = realpath(parent_path, NULL);
	real_child_path = realpath(child_path, NULL);

	if (!real_parent_path || !real_child_path)
		return NULL;

	/* Check to see that the child path starts with the parent path */
	if (strstr(real_child_path, real_parent_path) == real_child_path)
		return real_child_path;
	else
		return NULL;
}

#define HEADER_ALLOC_MAX (256*50)
#define HEADER_ALLOC_INC 256
#define HEADER_FUDGE 5
/* Header fudge is used because when libwebsockets writes headers for us we
 * don't really know how much space it's using in addition to the header name
 * and value so this is just a guess "fudge factor" */
#define HEADER_FUDGE 5

#define send_response_cleanup() \
	do { \
		free(mimetype); \
	} while (0)

static enum SohmuError send_file_response(
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	struct HttpResponse *response,
	const char *headers,
	const int len_headers
) {
	char *mimetype;
	int result_code;

	/* refuse to serve files we don't understand */
	mimetype = get_mimetype(response->resource_path);

	if (!mimetype) {
		lwsl_err("Unknown mimetype for %s\n", response->resource_path);
		libwebsockets_return_http_status(
			context, wsi,
			HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL
		);
		send_response_cleanup();
		return UNSUPPORTED_MEDIA_TYPE;
	}

	/* Finally, serve out our file and headers */
	result_code = libwebsockets_serve_http_file(
		context, wsi, response->resource_path,
		mimetype, headers, len_headers
	);

	if (
		result_code < 0 ||
		((result_code > 0) && lws_http_transaction_completed(wsi))
	) {
		/* error or can't reuse connection: close the socket */
		send_response_cleanup();
		return CANT_REUSE;
	}

	send_response_cleanup();
	return OK;
}

static enum SohmuError add_response_header(
	struct HttpResponse *response,
	const char *name, const char *value
) {
	/* We allocate headers 4 at a time */
	int alloc = (response->num_headers + 3) & ~3;
	if (alloc == response->num_headers) {
		/* Create room for me headers since we have exhausted our current size */
		alloc = (response->num_headers + 4) & ~3;
		if (response->num_headers == 0)
			response->headers =
				malloc(sizeof(struct Header)*alloc);
		else
			response->headers =
				realloc(response->headers, sizeof(struct Header)*alloc);

		if (!response->headers)
			return OUT_OF_MEMORY;
	}
	response->headers[response->num_headers].name = malloc(strlen(name)+1);
	strcpy(response->headers[response->num_headers].name, name);
	response->headers[response->num_headers].value = malloc(strlen(value)+1);
	strcpy(response->headers[response->num_headers++].value, value);

	return OK;
}

/* This is a redirect response. We handle it in a special way here because when
 * responding with a file libwebsockets will just respond with a status of 200.
 * Ideally both of those kinds of responses should be combined into common
 * response logic but this will do for now. */
static enum SohmuError send_redirect_response(
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	struct HttpResponse *response
) {
	int bytes_written;
	unsigned char *buffer = malloc(4096);
	unsigned char *buff_pointer, *buff_end_pointer;

	buff_pointer = buffer + LWS_SEND_BUFFER_PRE_PADDING;
	buff_end_pointer =
		buff_pointer + 4096 - LWS_SEND_BUFFER_PRE_PADDING;

	if (lws_add_http_header_status(
			context, wsi,
			302, &buff_pointer,
			buff_end_pointer
	)) return FAIL;

	if (lws_add_http_header_by_token(
			context, wsi,
			WSI_TOKEN_HTTP_LOCATION,
			(unsigned char *)response->redirect_location,
			strlen(response->redirect_location), &buff_pointer, buff_end_pointer
	)) return FAIL;
	if (lws_finalize_http_header(context, wsi, &buff_pointer, buff_end_pointer))
		return FAIL;

	bytes_written = libwebsocket_write(
		wsi,
		buffer + LWS_SEND_BUFFER_PRE_PADDING,
		buff_pointer - (buffer + LWS_SEND_BUFFER_PRE_PADDING),
		LWS_WRITE_HTTP_HEADERS
	);

	free(buffer);

	return OK;
}

/* Set the headers on the response */
static enum SohmuError set_file_headers(
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	struct HttpResponse *response,
	/* This is an actual string that is built up by libwebsockets for its use
	 * internally whereas the headers on *response are an array of key value
	 * pairs which will have come from one of the routes. */
	char **headers,
	int *headers_len
) {
	int i, n, len = 0;
	int alloc = HEADER_ALLOC_INC;
	
	if (response->num_headers)
		*headers = malloc(alloc);
	else {
		*headers = NULL;
		return OK;
	}

	/* Are there any headers that we need to set on the response? */
	for (i = 0; i < response->num_headers; ++i) {
		/* Pointer to the first position after the last written header in
		 * *headers */
		char *headers_after = *headers + len;

		/* Figure out how much space we need to write these headers */
		alloc =
			(
				len +
				strlen(response->headers[i].name) +
				strlen(response->headers[i].value) +
				HEADER_FUDGE + HEADER_ALLOC_INC
			) & ~HEADER_ALLOC_INC;

		if (alloc < HEADER_ALLOC_MAX)
			*headers = realloc(*headers, alloc);
		else {
			*headers = NULL;
			return EXCEEDED_MAX_HEADER_SIZE;
		}

		if (
			lws_add_http_header_by_name(context, wsi, 
				(unsigned char *)response->headers[i].name, 
				(unsigned char *)response->headers[i].value,
				strlen(response->headers[i].value), (unsigned char **)&headers_after,
				(unsigned char *)*headers + alloc 
			)
		) {
			*headers = NULL;
			return COULD_NOT_SET_HEADERS;
		}
		len = headers_after - *headers;
	}

	/* If we got here and didn't bail then everything worked out. We now just
	 * need to tell the caller how long the headers are */
	*headers_len = len;
	return OK;
}

enum SohmuError route(
	struct HttpContext *http_ctx,
	struct HttpResponse **response,
	const char *in
) {
	int i;
	enum SohmuError status;

	if (http_ctx->num_routes == 0)
		return NO_REGISTERED_ROUTES;

	/* Iterate over our routes and see if any of them can handle the request */
	for (i = 0; i < http_ctx->num_routes; ++i) {
		status = http_ctx->routes[i](http_ctx, in, response);
		/* Here, either we found the route and maybe some error occurred.
		 * Either way we're finished looking. */
		if (status != ROUTE_NOT_FOUND)
			return status;
	}

	/* TODO: Maybe return something else? */
	return ROUTE_NOT_FOUND;
}

static char *rebuild_full_uri(struct libwebsocket *wsi, char *in) {
	long args_len, in_len;
	char *full_uri;

	args_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_URI_ARGS);
	in_len = strlen(in);
	if (!args_len)
		return NULL;
	full_uri = malloc(in_len + args_len + 1);
	strcpy(full_uri, in);
	full_uri[in_len] = '?';
	lws_hdr_copy(wsi, full_uri + in_len + 1, args_len + 1, WSI_TOKEN_HTTP_URI_ARGS);

	return full_uri;
}

#define http_request_cleanup() \
	do { \
		*response = NULL; \
	} while (0)

enum SohmuError http_request(
	struct HttpContext *http_ctx,
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	void *user,
	void *in, size_t len,
	/* We hand this back because we need to free it when we're notified that
	 * the transfer is complete */
	struct HttpResponse **response
) {
	int i, len_headers;
	char *headers, *mimetype, *request_uri, *tmp;
	enum SohmuError status;
	long arglen;

	tmp = rebuild_full_uri(wsi, in);
	if (tmp)
		request_uri = tmp;
	else
		request_uri = in;

	if (len < 1) {
		libwebsockets_return_http_status(
			context, wsi,
			HTTP_STATUS_BAD_REQUEST, NULL
		);
		http_request_cleanup();
		return BAD_REQUEST;
	}

	status = route(http_ctx, response, request_uri);
	if (status != ROUTE_FOUND) {
		fprintf(
			stderr, "An error occurred while routing: %s\n",
			err_msg(status)
		);

		return 0;
	}

	if ((*response)->resource_path) {
		status = set_file_headers(
			context,wsi, *response,
			&headers, &len_headers
		);
		if (status != OK) {
			http_request_cleanup();
			return status;
		}

		status = send_file_response(
			context, wsi, *response,
			headers, len_headers
		);
	}
	else if ((*response)->redirect_location){
		/* TODO: This is a headers only response which allows us to also send
		 * back a status. It's unfortunate that this is being handled totally
		 * separately at the moment. */
		if(status = send_redirect_response(context, wsi, *response))
			fprintf(stderr, "Error sending redirect: %s\n", err_msg(status));
	}

	if (tmp)
		free(request_uri);
	return OK;
}

static enum SohmuError try_to_reuse(struct libwebsocket *wsi) {
	enum SohmuError status;
	int return_code;

	return_code = lws_http_transaction_completed(wsi)?-1:0;
	if (return_code)
		return CANT_REUSE;

	return OK;
}

int callback_http(
	struct HttpContext *http_ctx,
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason, void *user,
	void *in, size_t len
) {
	enum SohmuError result;
	struct HttpSession *usr = user;

	if (reason == LWS_CALLBACK_HTTP) {
		result = http_request(
			http_ctx, context,
			wsi, usr, in, len,
			&(usr->response)
		);
		if (result != OK)
			fprintf(
				stderr,
				"Error requesting: %s, %s\n",
				usr->response->resource_path,
				err_msg(result)
			);
	}
	else if (reason == LWS_CALLBACK_HTTP_FILE_COMPLETION) {
		/* Cleanup the http response from the route */
		destroy_http_response(usr->response);
		/* kill the connection after we sent one file */
		return try_to_reuse(wsi);
	}

	return 0;
}

enum SohmuError http_init(
	struct HttpContext **http_ctx,
	struct RouteServices *services
) {
	if (!(*http_ctx = malloc(sizeof(struct HttpContext))))
		return OUT_OF_MEMORY;
	if(!((*http_ctx)->routes = malloc(NUM_ROUTES_INC))) {
		free(http_ctx);
		return OUT_OF_MEMORY;
	}
	(*http_ctx)->num_routes = 0;
	(*http_ctx)->services = services;

	return OK;
}

enum SohmuError http_destroy(struct HttpContext *http_ctx) {
	int i;
	/* Free all of the route callbacks */
	for (i = 0; http_ctx->num_routes; ++i)
		free(http_ctx->routes[i]);

	return OK;
}

enum SohmuError register_route(
	struct HttpContext *http_ctx,
	enum SohmuError (*route)(
		struct HttpContext *http_ctx,
		const char *request_uri,
		struct HttpResponse **response
	)
) {
	int route_alloc_size =
		(http_ctx->num_routes + NUM_ROUTES_INC) & ~NUM_ROUTES_INC;
	if (http_ctx->num_routes >= route_alloc_size)
		http_ctx->routes =
			realloc(http_ctx->routes, http_ctx->num_routes + NUM_ROUTES_INC);
	if(!http_ctx->routes)
		return OUT_OF_MEMORY;
	http_ctx->routes[http_ctx->num_routes++] = route; 

	return OK;
}

enum SohmuError init_http_response(struct HttpResponse **response_to_init) {
	*response_to_init = malloc(sizeof(struct HttpResponse));
	if (!*response_to_init)
		return OUT_OF_MEMORY;
	(*response_to_init)->resource_path = NULL;
	(*response_to_init)->redirect_location = NULL;
	(*response_to_init)->num_headers = 0;
	(*response_to_init)->headers = NULL;
	(*response_to_init)->status = 200;

	return OK;
}

enum SohmuError destroy_http_response(struct HttpResponse *response_to_free) {
	int i;

	if (response_to_free->resource_path)
		free(response_to_free->resource_path);
	if (response_to_free->redirect_location)
		free(response_to_free->redirect_location);
	for (i = 0; i < response_to_free->num_headers; ++i)
		free(response_to_free->headers + i);
	free(response_to_free);
}
