#ifndef __sock_ntop_h_
#define __sock_ntop_h_

#include "defines.h"

char* _sock_ntop(const SA*, socklen_t);
char* sock_ntop(const SA*, socklen_t);

#endif // __sock_ntop_h_