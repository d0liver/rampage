#include <stdio.h>
#include "err.h"
#include "defs.h"

#if DEBUG
void debug (const char *format, ...)
{
	va_list args;

	va_start(args, format);
	/* printf("\033[0;33m"); */
	vprintf(format, args);
	/* printf("\033[0m"); */
	va_end(args);
}
#else
void debug (const char *format, ...)
{
}
#endif
