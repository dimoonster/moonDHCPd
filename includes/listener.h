#ifndef __listener_h_
#define __listener_h_

#include <sys/socket.h>
#include <netinet/in.h>
#include "defines.h"

void listener(int, SA*, socklen_t);
ssize_t recvfrom_flags(int, void *, size_t, int *, SA *, socklen_t *, struct in_pktinfo *);
void interface_name(int, char*);
ssize_t sendwith_flags(int, void*, size_t, int*, struct in_pktinfo*);

#endif // __listener_h_