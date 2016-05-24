#ifndef HTTP_H
#include <stdlib.h>
#include <libwebsockets.h>
#include <mysql/mysql.h>
#include <sqlite3.h>

#define HTTP_H
#define RESOURCE_PATH "/home/david/cohmu"
#define INDEX_PAGE "/index.html"

/* This is an http response object that should come back from a route. */
struct HttpResponse {
	/* If there is a file that needs to be transmitted then we put the path to
	 * it here */
	char *resource_path;
	char *redirect_location;
	/* If there are headers that need to be set we put those here */
	struct Header *headers;
	int num_headers;
	int status;
};

struct Header {
	char *name;
	char *value;
};

struct Extension {
	char *file_ext;
	char *mimetype;
};

struct HttpContext {
	/* These are the http route handlers */
	enum RmpgErr (**routes)(
		const char *page_request_uri,
		struct HttpResponse **response
	);
	int num_routes;
};

struct HttpSession {
	struct HttpResponse *response;
};

int http_callback(
	struct libwebsocket_context *,
	struct libwebsocket *,
	enum libwebsocket_callback_reasons,
	void *, void *, size_t
);

/* Routes */
enum RmpgErr register_route(
	enum RmpgErr (*route)(
		const char *request_uri,
		struct HttpResponse **response
	)
);
char *verify_path(const char *parent_path, const char *child_path);

/* Response */
enum RmpgErr init_http_response(struct HttpResponse **response_to_init);
enum RmpgErr destroy_http_response(struct HttpResponse *response_to_free);
enum RmpgErr add_response_header(
	struct HttpResponse *response,
	const char *name, const char *value
);

/* Http Context */
enum RmpgErr http_init();

enum RmpgErr http_destroy();
#endif
