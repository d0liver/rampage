#include <stdlib.h>
#include <stdio.h>

#include "event_mgr.h"

/*
 * This is where we take assembled messages and do something useful with them
 * (like emit them into channels). Things above this level of abstraction
 * should have a fairly reasonable API to deal with.
 */

static void handle(char *msg) {
}
