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
	struct RouteServices *services;
	/* These are the http route handlers */
	enum SohmuError (**routes)(
		struct HttpContext *http_ctx,
		const char *page_request_uri,
		struct HttpResponse **response
	);
	int num_routes;
};

struct HttpSession {
	struct HttpResponse *response;
};

struct RouteServices {
	MYSQL *mysql;
	sqlite3 *sqlite;
};

int callback_http(
	struct HttpContext *http_ctx,
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len
);

/* Routes */
enum SohmuError register_route(
	struct HttpContext *http_ctx,
	enum SohmuError (*route)(
		struct HttpContext *http_ctx,
		const char *request_uri,
		struct HttpResponse **response
	)
);
char *verify_path(const char *parent_path, const char *child_path);

/* Response */
enum SohmuError init_http_response(struct HttpResponse **response_to_init);
enum SohmuError destroy_http_response(struct HttpResponse *response_to_free);
enum SohmuError add_response_header(
	struct HttpResponse *response,
	const char *name, const char *value
);

/* Http Context */
enum SohmuError http_init(
	struct HttpContext **http_ctx,
	struct RouteServices *services
);
enum SohmuError http_destroy(struct HttpContext *http_ctx);
enum SohmuError init_route_services(
	struct RouteServices **services,
	MYSQL *mysql,
	sqlite3 *sqlite
);
#endif
