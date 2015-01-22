#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "defines.h"
#include "dhcp_db.h"
#include "logg.h"
#include "mydhcp.h"
#include "listener.h"
#include "dhcp_route.h"

// функция инициализации соединения с БД
void dhcp_db_init() {
    db_conn = mysql_init(NULL);
    my_bool reconnect = 1;
    
    mysql_options(db_conn, MYSQL_OPT_RECONNECT, &reconnect);
    
    if(!mysql_real_connect(db_conn, confdata.db_server, confdata.db_user, confdata.db_password, confdata.db_name, 0, NULL, 0)) {
	logg_quit("dhcp_db init err: %s", mysql_error(db_conn));
    }
}

// Функция возвращает список опций, для лиза по указанному IP
dhcpv4_options_list*	dhcpv4_db_options(char *ip, dhcpv4_options_list	*options) {
    char query[MAXLINE];
    int opts=0;
    
    MYSQL_RES	*res;
    MYSQL_ROW	row;
    
    bzero(query, sizeof(query));
    snprintf(query, sizeof(query),
	"select options from leases where ipaddr = INET_ATON('%s')",
	ip
    );
    
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db db_options err: %s", mysql_error(db_conn));
	return NULL;
    }
    
    res = mysql_use_result(db_conn);
    if((row = mysql_fetch_row(res)) != NULL) {
	opts = atoi(row[0]);
    } else {
	logg_msg("dhcp_db db_options err: lease for ip %s not found", ip);
	return NULL;
    }
    mysql_free_result(res);
    
    bzero(query, sizeof(query));
    snprintf(query, sizeof(query),
	"select code,value,type from options_template_data where template=%i",
	opts
    );
    
    mysql_ping(db_conn);
    
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db db_options err: %s", mysql_error(db_conn));
	return NULL;
    }
    res = mysql_use_result(db_conn);
    
    while((row = mysql_fetch_row(res)) != NULL) {
	int code = atoi(row[0]);
	int type = atoi(row[2]);
	char* udata = row[1];
	
	switch(type) {
	    case DB_DHCP_OPTION_BYTE: {
		options = dhcpv4_addoption_byte(options, code, udata[0]);
	    }; break;
	    case DB_DHCP_OPTION_STRING: {
		options = dhcpv4_addoption_string(options, code, udata);
	    }; break;
	    case DB_DHCP_OPTION_IP_ADDR: {
		options = dhcpv4_addoption_ip(options, code, udata);
	    }; break;
	    case DB_DHCP_OPTION_IP_LIST: {
		options = dhcpv4_addoption_iplist2(options, code, udata);
	    }; break;
	    case DB_DHCP_OPTION_NUMBER: {
		unsigned long ldata;
		ldata = atol(udata);
		dhcpv4_addoption_long(options, code, ldata);
	    }; break;
	}
    }
    mysql_free_result(res);
    
    return options;
}


// функция взаимодействия события DHCP_REQUEST с базой данных
// параметры:
//	interface 	- идентификатор интерфейса, с которого пршёл запрос
//	reqip		- IP адрес запрашиваемый клиентом
//	chaddr		- физический адрес устройства которое запрашивает IP
int dhcpv4_request_ip(int interface, char* reqip, char* chaddr) {
    char query[MAXLINE];
    char intstr[MAXLINE];

    MYSQL_RES *res;
    MYSQL_ROW row;

    dhcpv4_db_clear_leases();
    
//    printf("int=%i, ip=%s, ch=%s\n", interface, reqip, chaddr);
    
    bzero(&query, sizeof(query));
    interface_name(interface, intstr);

    snprintf(query, sizeof(query),
	"select static,chaddr,routed from leases where interface='%s' and INET_ATON('%s')=ipaddr",
	intstr, reqip
    );
    
    mysql_ping(db_conn);
    
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db request_ip err: %s", mysql_error(db_conn));
	return DHCP_DECLINE;
    }
    
    res = mysql_use_result(db_conn);

    if((row = mysql_fetch_row(res)) != NULL) {
	int istatic = atoi(row[0]);
	ulong lease = confdata.default_lease_time;
	int routed = atoi(row[2]);
	char db_chaddr[MAXLINE]; bzero(&db_chaddr, sizeof(db_chaddr));
	
	snprintf(db_chaddr, sizeof(db_chaddr), "%s", row[1]);
	mysql_free_result(res);

	dhcpv4_options_list* opts = NULL;
	opts = dhcpv4_db_options(reqip, opts);

	dhcpv4_option* opt = dhcpv4_find_option(opts, DHCP_OPTION_DHCP_LEASE_TIME);
	if(opt != NULL) {
	    lease = nhgetl(opt->data);
	    free(opts);
	    free(opt);
	}
	
	char 		buftime[MAXLINE]; bzero(&buftime, sizeof(buftime));
	time_t		rawtime;
	struct tm*	timeinfo;
	
	time(&rawtime);
	rawtime += lease;
	timeinfo = localtime(&rawtime);
	
	strftime(buftime, sizeof(buftime), "%Y-%m-%d %H:%M:%S", timeinfo);
	
	bzero(query, sizeof(query));
	snprintf(query, sizeof(query),
	    "update leases set state=%i, chaddr='%s', offered_till='%s' where interface='%s'",
	    DB_DHCP_LEASE_ALLOCATED, chaddr, buftime, intstr
	    );

	if(mysql_query(db_conn, query)) {
	    logg_msg("dhcp_db request_ip err: %s", mysql_error(db_conn));
	    return DHCP_DECLINE;
	}
	
	if(istatic==1) dhcp_rtv4_delete(reqip, 32);
	if(routed!=0) dhcp_rtv4_add(reqip, 32, interface);

	return DHCP_ACK;
    } else {
	mysql_free_result(res);
    }
    
    return DHCP_NAK;
}

// функция взаимодействия события DHCP_DISCOVER с базой данных
// параметры:
//	interface	- системный номер интерфейса, с которого пришёл пакет
//	addr		- указатель, в который будет записан предлагаемый IP адрес
//	addrsize	- размер укзателя
//	chaddr		- физичский адрес устройства, которое запрашивает IP адрес
int dhcpv4_discover_ip(int interface, char* addr, size_t addrsize, char* chaddr) {
    char query[MAXLINE];
    char intstr[MAXLINE];
    
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    interface_name(interface, intstr);
    
    snprintf(query, sizeof(query),
	" select interfaces.id, interfaces.int_mask, interfaces.iplist, interfaces.options, interfaces.static, " \
        " interfaces.route, INET_NTOA(iplists.startip), INET_NTOA(iplists.endip), interfaces.ptp" \
        " from interfaces,iplists" \
        " where '%s' regexp interfaces.int_mask and iplists.id=interfaces.iplist order by interfaces.prio desc limit 0,1", intstr);

    mysql_ping(db_conn);
    
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db discover_ip err: %s", mysql_error(db_conn));
//	addr = NULL;
	return 0;
    }
    
    res = mysql_use_result(db_conn);

    if((row = mysql_fetch_row(res)) != NULL) {
        int id = atoi(row[0]);
//        int intmask = atoi(row[1]);
        int opts = atoi(row[3]);		// номер набора опций
        int ptp = atoi(row[8]);			// за интерфейсом 1 абонент
        int int_static = atoi(row[4]);		// адрес выдаётся и закрепляется за интерфесом навечно
        int int_route = atoi(row[5]);		// прописывать маршрут
        int iplist = atoi(row[2]);		// номер списка IP адресов
        
        char s_ip[IPV4_STR_LEN]; bzero(&s_ip, sizeof(s_ip));
        char e_ip[IPV4_STR_LEN]; bzero(&e_ip, sizeof(e_ip));
        
        memcpy(s_ip, row[6], sizeof(s_ip));	// начальный IP списка
        memcpy(e_ip, row[7], sizeof(e_ip));			// конечный IP списка

        char* yaddr;
        
        if(ptp == 0) int_static = 0;
        
        mysql_free_result(res);
        
        dhcpv4_db_clear_leases();

	yaddr = dhcpv4_db_offer_ip(iplist, s_ip, e_ip, intstr);
	
//	printf("yaddr=%s\n", yaddr);
        
    	if(!dhcpv4_db_store_offer(yaddr, id, intstr, opts, confdata.offer_lease_time, chaddr, int_static, int_route)) {
    	    return 0;
    	};
    	memcpy(addr, yaddr, addrsize);
    	yaddr=NULL;
    };
    
    return 1;
}

// Функция предлагает ип адрес находящейся в lease для интерфейса, если ИП адрес раньше не выделялся, то выделяет его
//	параметры:
//		iplist		- идентификатор ip списка в БД
//		s_ip		- начальный адрес списка
//		e_ip		- конечный адрес списка
//		interface	- интерфейс, с которого пришёл запрос. (пример = eth2.12.101)
char* dhcpv4_db_offer_ip(int iplist, char* s_ip, char* e_ip, char* interface) {
    char* yaddr=NULL;
    
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char query[MAXLINE];
    snprintf(query, sizeof(query), 
	"select INET_NTOA(ipaddr) from leases where ipaddr >= INET_ATON('%s') and ipaddr <= INET_ATON('%s') and interface = '%s'",
	s_ip, e_ip, interface);
	
    mysql_ping(db_conn);
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db offer_static_ip err: %s", mysql_error(db_conn));
	return NULL;
    }
    
    res = mysql_use_result(db_conn);
    if((row = mysql_fetch_row(res)) != NULL) {
	yaddr = (char*)malloc(IPV4_STR_LEN);
	bzero(yaddr, IPV4_STR_LEN);
	memcpy(yaddr, row[0], IPV4_STR_LEN);
    } else {
	yaddr = dhcpv4_db_offer_dyn_ip(iplist, s_ip, e_ip);
    }
    mysql_free_result(res);
    
    return yaddr;
}

// Функция выделения IP адреса из диапозона
//	параметры:
//		iplist	- идентификатор списка ип
//		s_ip	- первый ip списка
//		e_ip	- последний ip списка
char* dhcpv4_db_offer_dyn_ip(int iplist, char* s_ip, char* e_ip) {
    char* yaddr;

    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char	query[MAXLINE];
    
    snprintf(query, sizeof(query), "select ipaddr from leases where ipaddr >= INET_ATON('%s') and ipaddr <= INET_ATON('%s') order by ipaddr asc", s_ip, e_ip);

    mysql_ping(db_conn);
    
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db offer_dyn_ip err: %s", mysql_error(db_conn));
	return NULL;
    }
    
    res = mysql_use_result(db_conn);
    
    struct in_addr end_ip;
    bzero(&end_ip, sizeof(struct in_addr));
    
    unsigned long store_prev = 0;
    unsigned long free_ip = 0;
    
    while((row = mysql_fetch_row(res)) != NULL) {
	unsigned long current = strtoul(row[0], NULL, 0);
	
	if((store_prev==0)&&(current>dhcpv4_mysql_inet_aton(s_ip))) {
	    break;
	}
	
	if((store_prev==0)||((current-store_prev)==1)) {
	    store_prev = current;
	    continue;
	}
	
	free_ip = current - 1;
	break;
    }
    mysql_free_result(res);

    if(store_prev==0) free_ip = dhcpv4_mysql_inet_aton(s_ip);
    if(free_ip==0) free_ip = store_prev + 1;
    if(free_ip>dhcpv4_mysql_inet_aton(e_ip)) return NULL;

    yaddr = (char*)malloc(IPV4_STR_LEN);
    
    end_ip.s_addr = free_ip;
    yaddr = dhcpv4_mysql_inet_ntoa(end_ip);
    
    return yaddr;
}

// Функция сохранения предложения IP адреса в БД
//	параметры:
//		ip		- предлагаемый IP адрес
//		intmask		- идентификатор интерфейсной маски, по которой происходило выделение
//		interface	- интерфейс, с которого пришёл пакет
//		option		- идентификатор набор опций для передачи в DHCP
//		offertime	- сколько секунд действительно предложение
//		chaddr		- физический адрес устройства, приславшего запрос
//		istatic		- признак, что выделенный адрес является статичным для интерфейса
//		routed		- признак добавление маршрута на выданный адрес в указанный интерфейс
int dhcpv4_db_store_offer(char* ip, int intmask, char* interface, int options, int offertime, char* chaddr, int istatic, int routed) {
    char query[MAXLINE];
    bzero(query, sizeof(query));
    
    char buftime[MAXLINE];
    bzero(buftime, sizeof(buftime));
    
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    rawtime += offertime;

    timeinfo = localtime(&rawtime);
    strftime(buftime, sizeof(buftime), "%Y-%m-%d %H:%M:%S", timeinfo);

    mysql_ping(db_conn);
    
//    if(istatic) {
	snprintf(query, sizeof(query), 
	    "delete from leases where ipaddr=INET_ATON('%s')", ip
	);
	if(mysql_query(db_conn, query)) {
	    logg_msg("dhcp_db store_offer err: %s", mysql_error(db_conn));
	    return 0;
	}
	bzero(query, sizeof(query));
//    };
    
    snprintf(query, sizeof(query), 
	"insert into leases(ipaddr, int_mask, interface, options, offered_till, state, chaddr, static, nasid, routed) values(INET_ATON('%s'), %i, '%s', %i, '%s', %i, '%s', %i, %i, %i)",
	ip, intmask, interface, options, buftime, DB_DHCP_LEASE_OFFERED, chaddr, istatic, confdata.nasid, routed
    );

    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db store_offer err: %s", mysql_error(db_conn));
	return 0;
    }
    
    bzero(query, sizeof(query));
    snprintf(query, sizeof(query), 
	"insert into leases_log(ipaddr, int_mask, interface, options, offered_till, state, chaddr, static, nasid, routed) values(INET_ATON('%s'), %i, '%s', %i, '%s', %i, '%s', %i, %i, %i)",
	ip, intmask, interface, options, buftime, DB_DHCP_LEASE_OFFERED, chaddr, istatic, confdata.nasid, routed
    );
    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db store_offer_log err: %s", mysql_error(db_conn));
    }
    
    return 1;
}

// Функция удаляет устаревшие динамичные аренды, а также прописанные маршруты по этим арендам
void dhcpv4_db_clear_leases() {
    char query[MAXLINE];
    bzero(&query, sizeof(query));

    MYSQL_RES *res;
    MYSQL_ROW row;
    
    time_t rawtime;
    struct tm * timeinfo;
    char buftime[MAXLINE];
    bzero(&buftime, sizeof(buftime));

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buftime, sizeof(buftime), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    snprintf(query, sizeof(query), 
	"select INET_NTOA(ipaddr),state from leases where nasid=%i and static=0 and offered_till < '%s';",
	confdata.nasid, buftime
	);

    mysql_ping(db_conn);

    if(mysql_query(db_conn, query)) {
	logg_msg("dhcp_db clear_leases err: %s", mysql_error(db_conn));
	return;
    }
    
    res = mysql_use_result(db_conn);
    while((row = mysql_fetch_row(res))!=NULL) {
	char *ip = row[0];
	int state = atoi(row[1]);
	if(state == DB_DHCP_LEASE_ALLOCATED) dhcp_rtv4_delete(ip, 32);
    }
    mysql_free_result(res);
    
    bzero(query, sizeof(query));
    snprintf(query, sizeof(query), 
	"delete from leases where nasid=%i and static=0 and offered_till < '%s';",
	confdata.nasid, buftime
	);
	
    mysql_query(db_conn, query);
}

char* dhcpv4_mysql_inet_ntoa(struct in_addr ina) {
    static char buf[IPV4_STR_LEN];
    unsigned char *ucp = (unsigned char *)&ina;

    sprintf(buf, "%d.%d.%d.%d",
	ucp[3] & 0xff,
	ucp[2] & 0xff,
	ucp[1] & 0xff,
	ucp[0] & 0xff);
    return buf;
}

ulong dhcpv4_mysql_inet_aton(char* addr) {
    struct in_addr end_ip;
    bzero(&end_ip, sizeof(struct in_addr));
    if(inet_pton(AF_INET, addr, &end_ip) < 1) return 0;
    
    uchar* ptr = (uchar*)malloc(4);
    hnputl2(ptr, end_ip.s_addr);
    end_ip.s_addr = nhgetl(ptr);
    free(ptr);
    
    return end_ip.s_addr;
}
