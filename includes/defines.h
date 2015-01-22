#ifndef __defines_h_
#define __defines_h_

#define	MD_VERSION	"0.1"

#define	SA		struct sockaddr
#define	MAXLINE		4096	// Максимальная длина сообщения
#define	IPV4_STR_LEN	16

#define BROADCAST_ADDR	"255.255.255.255"
#define DEFAULT_CONFIG	"./moondhcp.conf"

#include <stdbool.h>

typedef struct	_confdebug {
    bool	info;
    bool	in_pkt;
    bool	datablock;
} confdebug;

struct _confdata {
    char	server_ip[IPV4_STR_LEN];
    int		nasid;
    int		offer_lease_time;
    int		default_lease_time;
    
    int		min_udp_size;
    
    bool	exec_external;
    char	external[MAXLINE];
    
    char	db_server[MAXLINE];
    char	db_user[MAXLINE];
    char	db_password[MAXLINE];
    char	db_name[MAXLINE];
    
    bool	daemon;
    confdebug	debug;
} confdata;


// Типы значений опций в БД
#define DB_DHCP_OPTION_BYTE	1
#define DB_DHCP_OPTION_STRING	2
#define	DB_DHCP_OPTION_IP_ADDR	3
#define DB_DHCP_OPTION_IP_LIST	4
#define DB_DHCP_OPTION_NUMBER	5

// Типы значений записей в leases
#define DB_DHCP_LEASE_OFFERED	1		// Адрес предложен
#define DB_DHCP_LEASE_ALLOCATED	2		// Адрес выделен клиенту

#endif // __defines_h_
