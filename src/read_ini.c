#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_ini.h"

static struct Option *parse_opt_line(char *line) {
	char *p = line;
	struct Option *option = malloc(sizeof(struct Option));
	int vlen;

	if (!option)
		goto option_oom;

	for (; *p != '='; ++p)
		if (p == '\0') {
			fprintf(stderr, "Unexpected end of option.\n");
			return NULL;
		}

	if (!(option->key = malloc(p - line + 1)))
		goto key_oom;

	memcpy(option->key, line, p - line);
	option->key[p - line] = '\0';
	/* Advance past the = */
	++p;

	/*
	 * FIXME: We assume unix line endings here so that in the resulting string
	 * the newline character is replaced with the nullterm.
	 */
	vlen = strlen(p);

	if (!(option->value = malloc(vlen)))
		goto value_oom;

	memcpy(option->value, p, vlen);
	option->value[vlen - 1] = '\0';

	return option;

value_oom:
	free(option->key);
key_oom:
	free(option);
option_oom:
	return NULL;
}

/* Returns an array of options */
void read_config(FILE *fp, void (**handle_opt)(struct Option **)) {
	char line[1024];
	struct Option **opts, **ptr;
	int line_count = 0;
	void (**tmp)(struct Option **) = handle_opt;

	/* First, count the number of lines in the file */
	while (fgets(line, 1024, fp))
		++line_count;

	rewind(fp);
	printf("Line count: %d\n", line_count);

	/* This could be slightly too large. Oh well. */
	opts = ptr = malloc(sizeof(struct Option *)*line_count+1);
	/* Gather our opts */
	for (; fgets(line, 1024, fp); ++ptr)
		*ptr = parse_opt_line(line);

	*ptr = NULL;

	/* Call the specified callbacks with the list of options */
	for (; *tmp; ++tmp)
		(*tmp)(opts);

	if (ferror(fp))
		fprintf(stderr, "An error occurred while reading the config file.\n");
}
