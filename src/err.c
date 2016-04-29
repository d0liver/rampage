#include <stdio.h>
#include "err.h"


#if DEBUG
void debug (const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
#else
void debug (const char *format, ...)
{
}
#endif
