#include <stdio.h>
#include <utils.h>
#include <stdarg.h>

void debug (const char *format, ...)
{
	va_list args;

	if (DEBUG)
	{
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}
