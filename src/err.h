#ifndef RMPG_ERR_H
#include <stdarg.h>

enum RmpgErr {
	OK = 0,
	ROUTE_FOUND,
	ERROR_OUT_OF_MEMORY,
	EMPTY_RESPONSE,
	ERROR_WRITING_TO_SOCKET,
	ERROR_PARTIAL_WRITE,
	ERROR_JSON_PARSE,
	ERROR_JSON_PACK,
	ERROR_UNSUPPORTED_MEDIA_TYPE,
	ERROR_CANT_REUSE,
	ERROR_FAIL,
	ERROR_EXCEEDED_MAX_HEADER_SIZE,
	ERROR_COULD_NOT_SET_HEADERS,
	ERROR_NO_REGISTERED_ROUTES,
	ERROR_ROUTE_NOT_FOUND,
	ERROR_BAD_REQUEST
};

void debug (const char *format, ...);
void http_debug(const char *format, ...);
char *out(char *in, long size);
char *term(char *in, long size);

#define RMPG_ERR_H
#endif
