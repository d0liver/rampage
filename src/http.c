#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "err.h"
#include "http.h"

#define MAX_REQUEST_URI_LEN
#define NUM_ROUTES_INC 16

static struct HttpContext http_ctx;
struct Extension extensions[] = {
	{".coffee", "text/coffeescript"},
	{".ico", "image/x-icon"},
	{".png", "image/png"},
	{".html", "text/html"},
	{".css", "text/css"},
	{".js", "text/javascript"},
	{".map", "text/javascript"},
	{".ttf", "application/octet-stream"}
};

static char *get_mimetype(const char *fname) {
	int len = strlen(fname), i;

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
char *verify_path(const char *parent_path, const char *child_path) {
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
/* Header fudge is used because when lwss writes headers for us we
 * don't really know how much space it's using in addition to the header name
 * and value so this is just a guess "fudge factor" */
#define HEADER_FUDGE 5

static enum RmpgErr send_file_response(
	struct lws *wsi,
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
		lws_return_http_status(
			wsi,
			HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL
		);
		return ERROR_UNSUPPORTED_MEDIA_TYPE;
	}

	http_debug("Mimetype: %s\n", mimetype);
	/* Finally, serve out our file and headers */
	lws_serve_http_file(
		wsi, response->resource_path,
		mimetype, NULL, 0
	);

	if (
		result_code < 0 ||
		((result_code > 0) && lws_http_transaction_completed(wsi))
	)
		/* error or can't reuse connection: close the socket */
		return ERROR_CANT_REUSE;

	return OK;
}

enum RmpgErr add_response_header(
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
			return ERROR_OUT_OF_MEMORY;
	}
	response->headers[response->num_headers].name = malloc(strlen(name)+1);
	strcpy(response->headers[response->num_headers].name, name);
	response->headers[response->num_headers].value = malloc(strlen(value)+1);
	strcpy(response->headers[response->num_headers++].value, value);

	return OK;
}

/* This is a redirect response. We handle it in a special way here because when
 * responding with a file lwss will just respond with a status of 200.
 * Ideally both of those kinds of responses should be combined into common
 * response logic but this will do for now. */
static enum RmpgErr send_redirect_response(
	struct lws *wsi,
	struct HttpResponse *response
) {
	int bytes_written;
	unsigned char *buffer = malloc(4096);
	unsigned char *buff_pointer, *buff_end_pointer;

	buff_pointer = buffer + LWS_SEND_BUFFER_PRE_PADDING;
	buff_end_pointer =
		buff_pointer + 4096 - LWS_SEND_BUFFER_PRE_PADDING;

	if (lws_add_http_header_status(
			wsi,
			302, &buff_pointer,
			buff_end_pointer
	)) return ERROR_FAIL;

	if (lws_add_http_header_by_token(
			wsi,
			WSI_TOKEN_HTTP_LOCATION,
			(unsigned char *)response->redirect_location,
			strlen(response->redirect_location), &buff_pointer, buff_end_pointer
	)) return ERROR_FAIL;
	if (lws_finalize_http_header(wsi, &buff_pointer, buff_end_pointer))
		return ERROR_FAIL;

	bytes_written = lws_write(
		wsi,
		buffer + LWS_SEND_BUFFER_PRE_PADDING,
		buff_pointer - (buffer + LWS_SEND_BUFFER_PRE_PADDING),
		LWS_WRITE_HTTP_HEADERS
	);

	free(buffer);

	return OK;
}

/* Set the headers on the response */
static enum RmpgErr set_file_headers(
	struct lws *wsi,
	struct HttpResponse *response,
	/* This is an actual string that is built up by lwss for its use
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
			return ERROR_EXCEEDED_MAX_HEADER_SIZE;
		}

		if (
			lws_add_http_header_by_name(
				wsi, 
				(unsigned char *)response->headers[i].name, 
				(unsigned char *)response->headers[i].value,
				strlen(response->headers[i].value), (unsigned char **)&headers_after,
				(unsigned char *)*headers + alloc 
			)
		) {
			*headers = NULL;
			return ERROR_COULD_NOT_SET_HEADERS;
		}
		len = headers_after - *headers;
	}

	/* If we got here and didn't bail then everything worked out. We now just
	 * need to tell the caller how long the headers are */
	*headers_len = len;
	return OK;
}

struct HttpResponse *route(const char *in) {
	int i;
	enum RmpgErr status;
	struct HttpResponse *response;

	if (http_ctx.num_routes == 0)
		/* TODO: This should actually return a 404 response. */
		return NULL;

	/* Iterate over our routes and see if any of them can handle the request */
	for (i = 0; i < http_ctx.num_routes; ++i) {
		response = http_ctx.routes[i](in);

		if (response)
			return response;
	}

	/* TODO: This should actually return a 404 response.*/
	return NULL;
}

static char *rebuild_full_uri(struct lws *wsi, char *in) {
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

struct HttpResponse *http_request(
	struct lws *wsi,
	void *user,
	void *in, size_t len
) {
	int i, len_headers;
	char *headers, *mimetype, *request_uri, *tmp;
	enum RmpgErr status;
	struct HttpResponse *response;
	long arglen;

	tmp = rebuild_full_uri(wsi, in);
	request_uri = (tmp)? tmp : in;
	http_debug("Request URI: %s\n", request_uri);

	if (len < 1) {
		lws_return_http_status(
			wsi,
			HTTP_STATUS_BAD_REQUEST, NULL
		);
		return NULL;
	}

	response = route(request_uri);
	http_debug("Resolved resource path: %s\n", response->resource_path);

	if (response->resource_path) {
		status = set_file_headers(
			wsi, response,
			&headers, &len_headers
		);

		if (status != OK)
			/* FIXME: Should return an internal server error */
			return NULL;

		status = send_file_response(
			wsi, response,
			headers, len_headers
		);
	}
	else if (response->redirect_location) {
		/* TODO: This is a headers only response which allows us to also send
		 * back a status. It's unfortunate that this is being handled totally
		 * separately at the moment. */
		if(status = send_redirect_response(wsi, response))
			/* FIXME: Should return an internal server error */
			return NULL;
	}

	if (tmp)
		free(request_uri);

	return response;
}

static enum RmpgErr try_to_reuse(struct lws *wsi) {
	enum RmpgErr status;
	int return_code;

	return_code = lws_http_transaction_completed(wsi)?-1:0;
	if (return_code)
		return ERROR_CANT_REUSE;

	return OK;
}

int http_callback(
	struct lws *wsi,
	enum lws_callback_reasons reason, void *user,
	void *in, size_t len
) {
	struct HttpSession *usr = user;

	if (reason == LWS_CALLBACK_HTTP) {
		/* FIXME: Could be OOM */
		usr->response = http_request(wsi, usr, in, len);
	}
	else if (reason == LWS_CALLBACK_HTTP_FILE_COMPLETION) {
		/* Kill the connection after we sent one file */
		try_to_reuse(wsi);
	}

	return 0;
}

enum RmpgErr http_init(void) {
	if(!(http_ctx.routes = malloc(NUM_ROUTES_INC)))
		return ERROR_OUT_OF_MEMORY;

	http_ctx.num_routes = 0;

	return OK;
}

enum RmpgErr register_route(Route *route) {
	int route_alloc_size =
		(http_ctx.num_routes + NUM_ROUTES_INC) & ~NUM_ROUTES_INC;
	if (http_ctx.num_routes >= route_alloc_size)
		http_ctx.routes =
			realloc(http_ctx.routes, http_ctx.num_routes + NUM_ROUTES_INC);
	if(!http_ctx.routes)
		return ERROR_OUT_OF_MEMORY;
	http_ctx.routes[http_ctx.num_routes++] = route; 

	return OK;
}

struct HttpResponse *init_http_response(void) {
	struct HttpResponse *response = malloc(sizeof(struct HttpResponse));
	if (!response)
		return NULL;
	response->resource_path = NULL;
	response->redirect_location = NULL;
	response->num_headers = 0;
	response->headers = NULL;
	response->status = 200;

	return response;
}

void destroy_http_response(struct HttpResponse *response) {
	int i;

	if (response->resource_path)
		free(response->resource_path);
	if (response->resource_path)
	if (response->redirect_location)
		free(response->redirect_location);
	for (i = 0; i < response->num_headers; ++i)
		free(response->headers + i);
	free(response);
}
