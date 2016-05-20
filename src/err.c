#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "defs.h"

#if DEBUG
#define PREFIX "\033[0;31m[Rampage]\033[0m "
void debug (const char *format, ...)
{
	va_list args;
	char *tmp = malloc(strlen(PREFIX) + strlen(format) + 1);

	strcpy(tmp, PREFIX);
	strcat(tmp, format);

	va_start(args, format);
	/* printf("\033[0;33m"); */
	vprintf(tmp, args);
	/* printf("\033[0m"); */
	va_end(args);
}
#else
void debug (const char *format, ...)
{
}
#endif
