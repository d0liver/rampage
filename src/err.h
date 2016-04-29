#ifndef ERR_H
#include <stdarg.h>

enum RmpgErr {
	OK = 0,
	OUT_OF_MEM,
	EMPTY_RESPONSE,
	ERROR_WRITING_TO_SOCKET,
	ERROR_PARTIAL_WRITE
};

void debug (const char *format, ...);

#define ERR_H
#endif
