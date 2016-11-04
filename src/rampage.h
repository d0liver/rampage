#ifndef RAMPAGE_H
#include "err.h"

/* Public api that's not part of core.c */
#include "evt_mgr.h"
#include "session.h"
#include "channel.h"
#include "http.h"
#include "read_ini.h"

void rmpg_init(void (*)(struct Option **));
void rmpg_loop(void);
void rmpg_cleanup(void);
#define RAMPAGE_H
#endif
