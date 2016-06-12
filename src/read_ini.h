#ifndef READ_INI_H

struct Option {
	char *key, *value;
};
void read_config(FILE *fp, void (**handle_opt)(struct Option *));

#define READ_INI_H
#endif
