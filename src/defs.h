#ifndef RMPG_DEFS_H

#define DEBUG 1
#define HTTP_DEBUG 1 
#define MAX_MESSAGE_QUEUE 32
#define MAX_SLOTS 100
/* Roughly 1MB (fits into whole pages) */
#define MAX_CHANNEL_BYTES (999424)
#define MAX_MKT_GROUP_RESULTS 10000
#define DEFAULT_DB_PATH "/sqlite/sde.sqlite"
#define DEFAULT_DB_MD5 "/sqlite/store.md5"
#define OKERR(condition, ...) \
	do { \
		if ((condition)) {\
			fprintf(stderr, __VA_ARGS__); \
			return NULL; \
		} \
	} while (0)
#define OKERRCODE(condition, ...) \
	do { \
		if ((condition)) {\
			fprintf(stderr, __VA_ARGS__); \
			return 1; \
		} \
	} while (0)

#define RMPG_DEFS_H
#endif
