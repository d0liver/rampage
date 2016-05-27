#ifndef RAMPAGE_H
#include "err.h"

/* Public api that's not part of core.c */
#include "evt_mgr.h"
#include "session.h"
#include "channel.h"
#include "http.h"

void rmpg_init(int argc, char **argv);
void rmpg_loop(void);
void rmpg_cleanup(void);
#define RAMPAGE_H
#endif
