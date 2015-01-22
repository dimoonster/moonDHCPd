#ifndef __logg_h_
#define __logg_h_

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>
#include "defines.h"

void logg_quit(const char*, ...);
void logg_msg(const char*, ...);

#endif // __logg_h_