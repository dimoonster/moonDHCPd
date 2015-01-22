#ifndef __DHCP_DB_H_
#define __DHCP_DB_H_

#include <mysql/mysql.h>
#include "mydhcp.h"

MYSQL*		db_conn;

void	dhcp_db_init();

int dhcpv4_discover_ip(int interface, char* addr, size_t addrsize, char* chaddr);
int dhcpv4_request_ip(int interface, char* reqip, char* chaddr);

char* dhcpv4_db_offer_dyn_ip(int iplist, char* s_ip, char* e_ip);
char* dhcpv4_db_offer_ip(int iplist, char* s_ip, char* e_ip, char* interface);
void dhcpv4_db_clear_leases();
int dhcpv4_db_store_offer(char* ip, int intmask, char* interface, int options, int offertime, char* chaddr, int istatic, int routed);
dhcpv4_options_list* dhcpv4_db_options(char *ip, dhcpv4_options_list *options);

char* dhcpv4_mysql_inet_ntoa(struct in_addr ina);
ulong dhcpv4_mysql_inet_aton(char* addr);

#endif // __DHCP_DB_H_