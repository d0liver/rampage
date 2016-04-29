/* This is some generic buffer logic to make working with/buffering strings
 * easier */
#ifndef BUFF_H
rmpg_buff_init(unsigned char **buff, unsigned int *size);
rmpg_add_buffer(unsigned char *buff, unsigned int *size);
rmpg_buff_destroy(unsigned char *buff);
#define BUFF_H
#endif
