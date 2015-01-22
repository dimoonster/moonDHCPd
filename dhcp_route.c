#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/route.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "dhcp_route.h"
#include "logg.h"
#include "defines.h"

dhcp_rt_request rtreq;
int rtfd;		// сокет

struct sockaddr_nl	la;
struct sockaddr_nl	pa;

struct msghdr		rtmsg;

int 			rtl;
struct rtattr*		rtap;
struct nlmsghdr*	nlp;

static int dhcp_rtv4_sendrequest() {
    struct iovec	iov;
    
    bzero(&pa, sizeof(pa));
    pa.nl_family = AF_NETLINK;
    
    bzero(&rtmsg, sizeof(rtmsg));
    rtmsg.msg_name = &pa;
    rtmsg.msg_namelen = sizeof(pa);
    
    iov.iov_base = &rtreq.nl;
    iov.iov_len = rtreq.nl.nlmsg_len;
    rtmsg.msg_iov = &iov;
    rtmsg.msg_iovlen = 1;
    
    return sendmsg(rtfd, &rtmsg, 0);
}

// инициализация
void dhcp_rtv4_init() {
    if((rtfd=socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
	logg_quit("NETLINK_ROUTE socket error %i: %s", errno, strerror(errno));
    };
    
    bzero(&la, sizeof(la));
    la.nl_family = AF_NETLINK;
    la.nl_pid = getpid();
    
    if(bind(rtfd, (SA*)&la, sizeof(la)) < 0) {
	logg_quit("NETLINK_ROUTE bind error %i: %s", errno, strerror(errno));
    };
}

// добавление маршрута
//	параметры:
//		addr - адрес
//		prefix - префикс адреса
//		index - индекс интерфеса, на который вешается маршрут
int dhcp_rtv4_add(char* addr, int prefix, int index) {
    int offset = 0;

    bzero(&rtreq, sizeof(rtreq));
    
    rtl = sizeof(struct rtmsg);
    rtap = (struct rtattr*)rtreq.buf;

    rtap->rta_type = RTA_DST;
    rtap->rta_len = sizeof(struct rtattr) + 4;
    inet_pton(AF_INET, addr, ((char*)rtap)+sizeof(struct rtattr));
    rtl += rtap->rta_len;
    offset += rtap->rta_len;
    
    rtap = (struct rtattr*)(((char*)rtap) + offset);
    rtap->rta_type = RTA_OIF;
    rtap->rta_len = sizeof(struct rtattr) + 4;
    memcpy(((char*)rtap)+sizeof(struct rtattr), &index, 4);
    rtl += rtap->rta_len;
    offset += rtap->rta_len;
    
    rtreq.nl.nlmsg_len = NLMSG_LENGTH(rtl);
    rtreq.nl.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
    rtreq.nl.nlmsg_type = RTM_NEWROUTE;
    
    rtreq.rt.rtm_family = AF_INET;
    rtreq.rt.rtm_table = RT_TABLE_MAIN;
    rtreq.rt.rtm_protocol = RTPROT_BOOT;
    rtreq.rt.rtm_scope = RT_SCOPE_LINK;
    rtreq.rt.rtm_type = RTN_UNICAST;
    
    rtreq.rt.rtm_dst_len = prefix;
    
    return dhcp_rtv4_sendrequest();
}

// удаление маршрута
//	параметры:
//		addr - адрес
//		prefix - префикс адреса
int dhcp_rtv4_delete(char* addr, int prefix) {
    int offset = 0;

    bzero(&rtreq, sizeof(rtreq));
    
    rtl = sizeof(struct rtmsg);
    rtap = (struct rtattr*)rtreq.buf;

    rtap->rta_type = RTA_DST;
    rtap->rta_len = sizeof(struct rtattr) + 4;
    inet_pton(AF_INET, addr, ((char*)rtap)+sizeof(struct rtattr));
    rtl += rtap->rta_len;
    offset += rtap->rta_len;
    
    rtreq.nl.nlmsg_len = NLMSG_LENGTH(rtl);
    rtreq.nl.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
    rtreq.nl.nlmsg_type = RTM_DELROUTE;
    
    rtreq.rt.rtm_family = AF_INET;
    rtreq.rt.rtm_table = RT_TABLE_MAIN;
    rtreq.rt.rtm_protocol = RTPROT_BOOT;
    rtreq.rt.rtm_scope = RT_SCOPE_LINK;
    rtreq.rt.rtm_type = RTN_UNICAST;
    
    rtreq.rt.rtm_dst_len = prefix;
    
    return dhcp_rtv4_sendrequest();
}

bool dhcp_rtv4_findroute(char* addr, int prefix) {
    char	buf[MAXLINE];
    int		nll = 0;
    int		rtn = 0;
    char	*p;
    struct rtmsg*	rtp;
    struct rtattr*	rtap;
    
    bzero(&buf, sizeof(buf));
    bzero(&rtreq, sizeof(rtreq));
    
    rtreq.nl.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    rtreq.nl.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    rtreq.nl.nlmsg_type = RTM_GETROUTE;

    rtreq.rt.rtm_family = AF_INET;
    rtreq.rt.rtm_table = RT_TABLE_MAIN;
    
    dhcp_rtv4_sendrequest();
    
    p = buf;
    while(1) {
	rtn = recv(rtfd, p, sizeof(buf)-nll, 0);
	nlp = (struct nlmsghdr*)p;
	
	if(nlp->nlmsg_type==NLMSG_DONE) break;
	
	p += rtn;
	nll += rtn;
	if((la.nl_groups & RTMGRP_IPV4_ROUTE) == RTMGRP_IPV4_ROUTE) break;
    }
    
    nlp = (struct nlmsghdr*)buf;
    for(;NLMSG_OK(nlp, nll);nlp=NLMSG_NEXT(nlp, nll)) {
	rtp = (struct rtmsg*)NLMSG_DATA(nlp);
	if(rtp->rtm_table != RT_TABLE_MAIN) continue;
	
	rtap = (struct rtattr*)RTM_RTA(rtp);
	rtl = RTM_PAYLOAD(nlp);
	for(;RTA_OK(rtap, rtl);rtap=RTA_NEXT(rtap,rtl)) {
	    switch(rtap->rta_type) {
		case RTA_DST: {
		    char	dst[IPV4_STR_LEN];
		    bzero(&dst, sizeof(dst));
		    inet_ntop(AF_INET, RTA_DATA(rtap), dst, sizeof(dst));
		    
		    if(!strncmp(dst, addr, sizeof(dst))&&(rtp->rtm_dst_len==prefix)) {
			return true;
		    };
		}; break;
	    }
	}
    }
    return false;
}