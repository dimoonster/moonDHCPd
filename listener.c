#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "listener.h"
#include "logg.h"
#include "sock_ntop.h"
#include "mydhcp.h"

void listener(int sockfd, SA *pcliaddr, socklen_t clilen) {
    const int 		on = 1;
    struct in_addr 	in_zero;
    socklen_t		len;
    int			flags;
    ssize_t		n;
    char		mesg[MAXLINE];
    struct in_pktinfo	pktinfo;
    char 		int_name[MAXLINE];
    struct dhcpv4_pkt	dhcpv4pkt;
    dhcpv4_options_list	*dhcpv4opts=NULL;
    

    if(setsockopt(sockfd, SOL_IP, IP_PKTINFO, &on, sizeof(on)) < 0) logg_quit("Error seting socket options");
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) logg_quit("Error seting socket options");
    bzero(&in_zero, sizeof(in_zero));	// IPv4 из нулей
    
    for(;;) {
	bzero(&dhcpv4pkt, sizeof(struct dhcpv4_pkt));
	bzero(&mesg, sizeof(mesg));
	len = clilen;
	flags = 0;
	logg_msg("waiting for connection");
	n = recvfrom_flags(sockfd, mesg, MAXLINE, &flags, pcliaddr, &len, &pktinfo);
	
	pid_t pid;
	pid = fork();
	
	if(pid == 0) {
	    interface_name(pktinfo.ipi_ifindex, int_name);
	    if(int_name == NULL) {
		logg_msg("received datagram from unknown interface. Skiping.");
	    } else if (n < confdata.min_udp_size) {
		logg_msg("received datagram to small. Skiping. Datagram size %d-byte, from %s on interface %s", n, sock_ntop(pcliaddr, len), int_name);
	    } else {
		logg_msg("%d-byte datagram from %s. Interface Index is %d. Interface name is %s", n, sock_ntop(pcliaddr, len), pktinfo.ipi_ifindex, int_name);
		dhcpv4opts = mydhcp_parse_ipv4_pkt(&dhcpv4pkt, n, mesg);
	    
		if(dhcpv4opts==NULL) {
		    logg_msg("opts list is null. skipping packet");
//		    continue;
		    _exit(3);
		};
	    
		if(dhcpv4pkt.op != DHCP_BOOTREQUEST) {
		    logg_msg("datagram is not bootp DHCP_BOOTREQUEST. skipping packet");
//		    continue;
		    _exit(3);
		};
	    
		dhcpv4_option* 	opt_option53 = NULL;
		if((opt_option53 = dhcpv4_find_option(dhcpv4opts, DHCP_OPTION_DHCP_MESSAGE_TYPE)) == NULL) {
		    logg_msg("error. option DHCP_OPTION_DHCP_MESSAGE_TYPE not found! skipping packet");
		    free(dhcpv4opts);
//		    continue;
		    _exit(3);
		};

		dhcpv4_raw_packet*	rawpkt = NULL;
	    
		switch(opt_option53->data[0]) {
		    case DHCP_DISCOVER: {
			logg_msg("DHCP QUERY: DHCP_DISCOVER");
			rawpkt = dhcpv4_process_discover(&dhcpv4pkt, dhcpv4opts, pktinfo.ipi_ifindex);
			if(confdata.debug.in_pkt==true) mydhcp_show_ipv4_pkt(&dhcpv4pkt);
		    }; break;
		
		    case DHCP_REQUEST: {
			logg_msg("DHCP QUERY: DHCP_REQUEST");
			rawpkt = dhcpv4_process_request(&dhcpv4pkt, dhcpv4opts, pktinfo.ipi_ifindex);
			if(confdata.debug.in_pkt==true) mydhcp_show_ipv4_pkt(&dhcpv4pkt);
		    }; break;
		
		    default: {
			logg_msg("DHCP QUERY: UNKNOWN");
//			if(confdata.debug.in_pkt==true) mydhcp_show_ipv4_pkt(&dhcpv4pkt);
			mydhcp_show_ipv4_pkt(&dhcpv4pkt);
		    }; break;
		};
	    
		if(rawpkt == NULL) {
		    logg_msg("error creating answer packet.");
		    free(dhcpv4opts);
//		    continue;
		    _exit(3);
		};
	    
		logg_msg("trying send datagram size=%d", rawpkt->rawsize);
		n = sendwith_flags(sockfd, rawpkt->rawdata, rawpkt->rawsize, &flags, &pktinfo);
		logg_msg("send datagram size=%d, errno=%i, errstr=%s", n, errno, strerror(errno));

//	    struct dhcpv4_pkt _temp_pkt;
//	    bzero(&_temp_pkt, sizeof(struct dhcpv4_pkt));
//	    mydhcp_parse_ipv4_pkt(&_temp_pkt, rawpkt->rawsize, (char*)rawpkt->rawdata);
//	    logg_msg("-- generated packet");
//	    mydhcp_show_ipv4_pkt(&_temp_pkt);
	    
/*	    dhcpv4_raw_packet* rawpkt = NULL;
	    rawpkt = dhcpv4_generate_rawpkt(&dhcpv4pkt, dhcpv4opts);
	    
	    if(rawpkt == NULL) {
		logg_msg("error generating dhcp raw packet");
		continue;
	    };
		    
	    
	    free(dhcpv4opts);
	    free(rawpkt);
*/	    
//	    show_options_list(dhcpv4opts);
//	    dhcpv4_option* option = NULL;
//	    if((option = dhcpv4_find_option(dhcpv4opts, 53)) != NULL) {
//		logg_msg("option 53 value = %02x", option->data[0]);
//	    } else {
//		logg_msg("Option not found");
//	    };
	    
//	    mydhcp_show_ipv4_pkt(&dhcpv4pkt);
	    };
	    _exit(3);
	};
    }
}

ssize_t recvfrom_flags(int fd, void* ptr, size_t nbytes, int* flagsp, SA *sa, socklen_t *salenptr, struct in_pktinfo *pkt) {
    struct msghdr msgh;
    struct cmsghdr *cmsg;
    struct iovec iov;
    ssize_t n;
    char cbuf[MAXLINE];
    
    bzero(&msgh, sizeof(struct msghdr));
    iov.iov_base = ptr;
    iov.iov_len = nbytes;
    msgh.msg_control = cbuf;
    msgh.msg_controllen = sizeof(cbuf);
    msgh.msg_name = sa;
    msgh.msg_namelen = *salenptr;
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_flags = 0;
    
    if((n = recvmsg(fd, &msgh, *flagsp)) < 0) {
	return n;
    }
    
    if(salenptr) *salenptr = msgh.msg_namelen;
    
    for(cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
	if(cmsg->cmsg_type == IP_PKTINFO) {
	    struct in_pktinfo *pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
	    memcpy(pkt, pktinfo, sizeof(pkt));
	    break;
	}
    }
    return n;
}

ssize_t sendwith_flags(int fd, void* ptr, size_t nbytes, int* flagsp, struct in_pktinfo *pkt) {
    struct msghdr	msgh;
    struct cmsghdr	*cmsg;
    struct iovec	iov;
    char		cbuf[MAXLINE];
    struct in_pktinfo*	pktinfo;
    struct in_addr	broadcast;
    struct in_addr	brd_zero;
    ssize_t		n;
    struct sockaddr_in	brd;
        
    brd.sin_family	= AF_INET;
    brd.sin_addr.s_addr	= inet_addr(BROADCAST_ADDR);
    brd.sin_port	= htons(68);
    
    inet_aton(BROADCAST_ADDR, &broadcast);
    inet_aton("0.0.0.0", &brd_zero);

    bzero(&msgh, sizeof(struct msghdr));
    iov.iov_base = ptr;
    iov.iov_len = nbytes;

    msgh.msg_name = &brd;
    msgh.msg_namelen = sizeof(brd);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    
    msgh.msg_control = cbuf;
    msgh.msg_controllen = CMSG_SPACE(sizeof(struct in_pktinfo));
    
    cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
    cmsg->cmsg_level = SOL_IP;
    cmsg->cmsg_type = IP_PKTINFO;

    pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsg);

    pktinfo->ipi_ifindex = pkt->ipi_ifindex;
    pktinfo->ipi_addr = broadcast;
    pktinfo->ipi_spec_dst = brd_zero;
    
    n = sendmsg(fd, &msgh, *flagsp);
    
    return n;
}

void interface_name(int index, char* interface) {
    int i = -1;
    char int_name[MAXLINE];
        
    struct if_nameindex *interfaceArray = NULL;
    interfaceArray = if_nameindex();
    
    if(interfaceArray != NULL) {
	while(interfaceArray[++i].if_index!=0) {
	    if(interfaceArray[i].if_index == index) {
		snprintf(int_name, sizeof(int_name), "%s", interfaceArray[i].if_name);
		memcpy(interface, int_name, MAXLINE);
	    }
	}
	if_freenameindex(interfaceArray);
	interfaceArray = NULL;
    } else {
	logg_msg("if_nameindex() failed with errno = %d %s\n", errno,strerror(errno));
	interface = NULL;
    }
    
    return;
}