#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sock_ntop.h"
#include "defines.h"
#include "logg.h"

char* _sock_ntop(const SA* sa, socklen_t salen) {
    static char str[MAXLINE];
    
    switch(sa->sa_family) {
	case AF_INET: {
	    struct sockaddr_in *sin = (struct sockaddr_in*)sa;
	    if(inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
		return NULL;
	    return str;
	}
	case AF_INET6: {
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)sa;
	    if(inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
		return NULL;
	    return str;
	}
	default:	{
	    snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d", 
			sa->sa_family, salen);
	    return str;
	}
    }
    return NULL;
}

char* sock_ntop(const SA* sa, socklen_t salen) {
    char *ptr;
    if((ptr = _sock_ntop(sa, salen)) == NULL)
	logg_msg("sock_ntop error");
	
    return ptr;
}
