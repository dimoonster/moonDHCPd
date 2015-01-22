#ifndef __DHCP_ROUTE_H_
#define __DHCP_ROUTE_H_

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "defines.h"

typedef struct {
    struct nlmsghdr	nl;
    struct rtmsg	rt;
    char		buf[MAXLINE];
} dhcp_rt_request;

void dhcp_rtv4_init();			// Инициализация RT сокета
int dhcp_rtv4_add(char*, int, int);	// Добавление маршрута
int dhcp_rtv4_delete(char*, int);	// удаление маршрута
bool dhcp_rtv4_findroute(char* addr, int prefix);	// поиск маршрута в таблице

#endif // __DHCP_ROUTE_H_
