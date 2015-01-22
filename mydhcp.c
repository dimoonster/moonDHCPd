#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "defines.h"
#include "mydhcp.h"
#include "logg.h"
#include "dhcp_route.h"
#include "dhcp_db.h"
#include "listener.h"

char optmagic[4] = { 0x63, 0x82, 0x53, 0x63 };

typedef struct rawBootp	rawBootp;
struct rawBootp
{
    uchar	op;			/* opcode */
    uchar	htype;			/* hardware type */
    uchar	hlen;			/* hardware address len */
    uchar	hops;			/* hops */
    uchar	xid[4];			/* a random number */
    uchar	secs[2];		/* elapsed since client started booting */
    uchar	flags[2];
    uchar	ciaddr[4];		/* client IP address (client tells server) */
    uchar	yiaddr[4];		/* client IP address (server tells client) */
    uchar	siaddr[4];		/* server IP address */
    uchar	giaddr[4];			/* gateway IP address */
    uchar	chaddr[DHCP_CHADDR_LEN];	/* client hardware address */
    char	sname[DHCP_SNAME_LEN];		/* server host name (optional) */
    char	file[DHCP_FILE_LEN];		/* boot file name */
    uchar	optmagic[4];
    char	optdata[MAXLINE-236];
};

// функция по переводу 4-ёх байтовой строки в число
ulong nhgetl(uchar *ptr) {
    return ((ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3]);
}

ulong nhgetl2(uchar *ptr) {
    return ((ptr[3]<<24) | (ptr[2]<<16) | (ptr[1]<<8) | ptr[0]);
}

// функция по переводу 2-ух байтовой строки в число
ushort nhgets(uchar *ptr) {
    return ((ptr[0]<<8) | ptr[1]);
}

// функция по переводу числа в 2-ух байтовую строку
void hnputs(uchar *ptr, ushort val) {
    ptr[0] = val>>8;
    ptr[1] = val;
}

void hnputs2(uchar *ptr, ushort val) {
    ptr[1] = val>>8;
    ptr[0] = val;
}

// функция по переводу числа в 4-ёх байтовую строку
void hnputl(uchar *ptr, ulong val) {
    ptr[0] = val>>24;
    ptr[1] = val>>16;
    ptr[2] = val>>8;
    ptr[3] = val;
}

void hnputl2(uchar *ptr, ulong val) {
    ptr[3] = val>>24;
    ptr[2] = val>>16;
    ptr[1] = val>>8;
    ptr[0] = val;
}

// функция по переводу разобранного dhcp пакета и списка опций в вид,
// готовый для передачи по сети
//	параметры:
//		pkt	- разобраный пакет
//		_opts	- список опций
dhcpv4_raw_packet* dhcpv4_generate_rawpkt(struct dhcpv4_pkt* pkt, dhcpv4_options_list* _opts) {
    uchar* data = NULL;
    dhcpv4_raw_options* opts = NULL;
    dhcpv4_raw_packet* raw = NULL;
    rawBootp* bp = NULL;
    
    size_t	raw_size;
    
    opts = dhcpv4_generate_rawoptions(_opts);
    
    // Проверим наличие raw опций, и их размер
    if(opts == NULL) return NULL;
    if(opts->rawsize > MAXLINE-236) return NULL;

    if(confdata.debug.datablock==true) {
	int i;
	printf("Datablock size %i:\n", (unsigned int)opts->rawsize);
        for(i=0; i<opts->rawsize; i++) printf("%02x", opts->rawdata[i]);
        printf("\nEndDatablock\n");
    }

    bp = (rawBootp*)malloc(sizeof(rawBootp));
    
    // Заполним 236 байт пакета
    bp->op		= pkt->op;
    bp->htype		= pkt->htype;
    bp->hlen		= pkt->hlen;
    bp->hops		= 0;
    
    hnputl(bp->xid, pkt->xid);

    hnputs(bp->secs, 0);
    hnputs(bp->flags, pkt->flags);
    
    hnputl2(bp->ciaddr, pkt->ciaddr.s_addr);
    hnputl2(bp->yiaddr, pkt->yiaddr.s_addr);
    hnputl2(bp->siaddr, pkt->siaddr.s_addr);
    hnputl2(bp->giaddr, pkt->giaddr.s_addr);

    memcpy(bp->chaddr, pkt->chaddr, sizeof(bp->chaddr));
    memcpy(bp->sname, pkt->sname, sizeof(bp->sname));
    memcpy(bp->file, pkt->file, sizeof(bp->file));
    
    raw_size = 236+4;
    
    // Заполним опции dhcp
    memcpy(bp->optmagic, optmagic, sizeof(bp->optmagic));
    bzero(bp->optdata, sizeof(bp->optdata));
    memcpy(bp->optdata, opts->rawdata, opts->rawsize);
    raw_size += opts->rawsize;
    
    data = (uchar*)malloc(raw_size);
    data = (uchar*)bp;

    raw = (dhcpv4_raw_packet*)malloc(sizeof(dhcpv4_raw_packet));
    raw->rawsize = raw_size;
    raw->rawdata = data;
    return raw;
}

// функция по переводу списка опция в вид, готовый для передачи по сети
//	параметры:
//		_opts	- список опций
dhcpv4_raw_options* dhcpv4_generate_rawoptions(dhcpv4_options_list* _opts) {
    dhcpv4_raw_options *raw = NULL;
    dhcpv4_options_list* opts = NULL;
    uchar* data;
    size_t datalen=MAXLINE;
    int pos=-1;
    
    opts = _opts;
    data = (uchar*)malloc(datalen);
    bzero(data, datalen);
    
    while(opts) {
	// Если выходим за рамки, то выходим из цикла. 
	//	datalen-1 - резервируем последний байт на DHCP_OPTION_END
	//	opts->option.len+2 - добавляем 2 байта на код опции и длину опции
	if(++pos > datalen-1) break;
	if(datalen-1 < pos+opts->option.len+2) break;
	
	data[pos++] = opts->option.code;
	data[pos++] = opts->option.len;
	
	int j;
	for(j=0;j<opts->option.len;j++) {
	    data[pos+j] = opts->option.data[j];
	};
	pos += opts->option.len-1;
	
	opts = opts->nextoption;
    }
    
    pos++;    
    data[pos] = DHCP_OPTION_END;
    pos++;

    if(confdata.debug.datablock==true) {
        int i;
        printf("Datablock size %i:\n", pos);
        for(i=0; i<pos; i++) printf("%02x", data[i]);
        printf("\nEndDatablock\n");
    }

    raw = (dhcpv4_raw_options*)malloc(sizeof(dhcpv4_raw_options));
    raw->rawdata = NULL;
    raw->rawsize = pos;
    
    raw->rawdata = data;

    return raw;
}

// функция по разбору raw данных (пришебших по сети)
//	параметры:
//		dhcppkt	- структура, которая будет заполнена разобраными данными пакета
//		pktlen	- длина raw данных
//		pkt	- raw данные
//	возвращаемое значение:
//		список опций из пакета
dhcpv4_options_list* mydhcp_parse_ipv4_pkt(struct dhcpv4_pkt* dhcppkt, int pktlen, char* pkt) {
    rawBootp *bp;
    char	cipaddr[15];
    uchar	p[4];
    int		len, code;
    dhcpv4_options_list*	opts = NULL;
    
    bp = (rawBootp*)malloc(sizeof(rawBootp));
    bzero(bp, sizeof(rawBootp));
    
    bp=(rawBootp*)pkt;
    if(pktlen < bp->optmagic - (uchar*)pkt) {
	logg_msg("dhcp parse: short bootp packet");
	return NULL;
    }
    
//    if(bp->op != DHCP_BOOTREQUEST) {
//	logg_msg("dhcp parse: invalid op type");
//	return NULL;
//    }
    
    dhcppkt->op			= bp->op;
    dhcppkt->htype		= bp->htype;
    dhcppkt->hlen		= bp->hlen;
    dhcppkt->hops		= bp->hops;
    
    dhcppkt->xid		= nhgetl(bp->xid);

    dhcppkt->secs		= nhgets(bp->secs);
    dhcppkt->flags		= nhgets(bp->flags);
    
    snprintf(cipaddr, sizeof(cipaddr), "%d.%d.%d.%d", bp->ciaddr[0], bp->ciaddr[1], bp->ciaddr[2], bp->ciaddr[3]);
    inet_aton(cipaddr, &dhcppkt->ciaddr);

    snprintf(cipaddr, sizeof(cipaddr), "%d.%d.%d.%d", bp->yiaddr[0], bp->yiaddr[1], bp->yiaddr[2], bp->yiaddr[3]);
    inet_aton(cipaddr, &dhcppkt->yiaddr);

    snprintf(cipaddr, sizeof(cipaddr), "%d.%d.%d.%d", bp->siaddr[0], bp->siaddr[1], bp->siaddr[2], bp->siaddr[3]);
    inet_aton(cipaddr, &dhcppkt->siaddr);

    snprintf(cipaddr, sizeof(cipaddr), "%d.%d.%d.%d", bp->giaddr[0], bp->giaddr[1], bp->giaddr[2], bp->giaddr[3]);
    inet_aton(cipaddr, &dhcppkt->giaddr);
    
    memcpy(dhcppkt->chaddr, bp->chaddr, sizeof(dhcppkt->chaddr));
    memcpy(dhcppkt->sname, bp->sname, sizeof(dhcppkt->sname));
    memcpy(dhcppkt->file, bp->file, sizeof(dhcppkt->file));
    
    pktlen -= bp->optmagic - (uchar*)pkt;
    if(pktlen < 4) {
	logg_msg("dhcp parse: not option data");
	return NULL;
    }
    
    memcpy(p, bp->optmagic, sizeof(p));
    if(memcmp(optmagic, p, 4) != 0) {
	logg_msg("dhcp parse: bad opt magic: %x %x %x %x", p[0], p[1], p[2], p[3]);
	return NULL;
    };
    
    bzero(&dhcppkt->options, sizeof(dhcppkt->options));
    
    memcpy(dhcppkt->magic, bp->optmagic, sizeof(dhcppkt->magic));
    memcpy(dhcppkt->options, bp->optdata, pktlen);

    dhcppkt->optslen = pktlen;

    int n = dhcppkt->optslen;
    uchar* pdata = (uchar*)malloc(dhcppkt->optslen);
    memcpy(pdata, dhcppkt->options, dhcppkt->optslen);

    if(confdata.debug.datablock==true) {
        int i;
        printf("Datablock size %i:\n", dhcppkt->optslen);
        for(i=0; i<n; i++) printf("%02x", pdata[i]);
        printf("\nEndDatablock\n");
    }
    
    while(n > 0) {
	code = *pdata++;
	n--;
	
	if(code == DHCP_OPTION_PAD) continue;
	if(code == DHCP_OPTION_END) break;
	if(n == 0) break;
	
	len = *pdata++;
	n--;
	if(len > n) break;
	
	dhcpv4_option option;
	option.code = code;
	option.len  = len;
	option.data = (uchar*)malloc(len);
	memcpy(option.data, pdata, len);
	
	opts = dhcpv4_addoption(opts, option);
	
	pdata += len;
	n -= len;
    }

    pdata = NULL;
    
    return opts;
}

// функция отображает список опций
//	параметры
//		_opts	- список опций
void show_options_list(const dhcpv4_options_list* _opts) {
    dhcpv4_options_list* opts = NULL;

    char	optstr[MAXLINE];
    char	optvalue[MAXLINE];
    int		i;
    
    opts = _opts;

    while(opts) {
	bzero(optstr, sizeof(optstr));
	bzero(optvalue, sizeof(optvalue));
	
	for(i=0;i<opts->option.len;i++) {
	    snprintf(optvalue+i*2, (sizeof(optvalue)-i)*2, "%02x", opts->option.data[i]);
	};
	
	logg_msg("%s(%d): 0x%s", 
		optiontostring(opts->option.code, optstr, sizeof(optstr)), opts->option.code, optvalue);
	opts = opts->nextoption;
    }
}

// функция по поиску опции в списке по ее коду
//	параметры:
//		_opts	- список опций
//		code	- код опции
dhcpv4_option* dhcpv4_find_option(const dhcpv4_options_list* _opts, int code) {
    dhcpv4_option*		retval = NULL;
    dhcpv4_options_list*	opts = NULL;
    
    opts = _opts;
    while(opts) {
	if(opts->option.code == code) {
	    retval = &opts->option;
	}
	opts = opts->nextoption;
    }
    
    return retval;
}

// функция добавления опции в список
//	параметры
//		opts	- указаитель на список
//		option	- добавляемая опция
dhcpv4_options_list* dhcpv4_addoption(dhcpv4_options_list* opts, dhcpv4_option option) {

    dhcpv4_options_list* element = (dhcpv4_options_list*)malloc(sizeof(dhcpv4_options_list));
    element->nextoption = NULL;

    element->option = option;

    if(opts==NULL) {
        opts = element;
    } else {
        dhcpv4_options_list* _elem=NULL;
        _elem = opts;
        while(_elem->nextoption) {
	    _elem = _elem->nextoption;
	};
	_elem->nextoption = element;
    }
    
    return opts;
}

// функция перевода физического адреса устройства из dhcp пакета в строку
//	параметры:
//		dhcppkt	- разобраный dhcp пакет
//		cmac	- указатель на строку, куда будет сохранён результат
//		cmaclen	- длина строки
char* convert_chaddrtostring(const struct dhcpv4_pkt* dhcppkt, char* cmac, size_t cmaclen) {
    int i;

    for(i=0; i<dhcppkt->hlen;i++) {
	snprintf(cmac+i*2, (cmaclen-i)*2, "%02x", dhcppkt->chaddr[i]);
    }
    
    return cmac;
}

// функция отображения в человеческом виде разобранного DHCP пакета
//	параметры:
//		dhcppkt	- разобраный dhcp пакет
void mydhcp_show_ipv4_pkt(const struct dhcpv4_pkt* dhcppkt) {
    char	cmac[32];
    int		len, i, code;
    
    logg_msg("----- show ipv4 dhcp packet -----");
    switch(dhcppkt->op) {
	case DHCP_BOOTREQUEST: logg_msg("op = BOOTREQUEST"); break;
	case DHCP_BOOTREPLY: logg_msg("op = BOOTREPLY"); break;
	default: logg_msg("op = UNKNOWN"); break;
    }
    switch(dhcppkt->htype) {
	case DHCP_HTYPE_ETHER: logg_msg("htype = HTYPE_ETHER"); break;
	case DHCP_HTYPE_IEEE802: logg_msg("htype = HTYPE_IEEE802"); break;
	case DHCP_HTYPE_FDDI: logg_msg("htype = HTYPE_FDDI"); break;
	default: logg_msg("htype = UNKNOWN"); break;
    }
    logg_msg("hlen = %d", dhcppkt->hlen);
    logg_msg("hops = %d", dhcppkt->hops);
    logg_msg("xid = %x", dhcppkt->xid);
    logg_msg("secs = %d", dhcppkt->secs);
    logg_msg("flags = %x", dhcppkt->flags);
    
    logg_msg("ciaddr = %s", inet_ntoa(dhcppkt->ciaddr));
    logg_msg("yiaddr = %s", inet_ntoa(dhcppkt->yiaddr));
    logg_msg("siaddr = %s", inet_ntoa(dhcppkt->siaddr));
    logg_msg("giaddr = %s", inet_ntoa(dhcppkt->giaddr));
    
    logg_msg("chaddr = %s", convert_chaddrtostring(dhcppkt, cmac, sizeof(cmac)));
    logg_msg("sname = %s", dhcppkt->sname);
    logg_msg("file = %s", dhcppkt->file);

    int n = dhcppkt->optslen;
    uchar* p = (uchar*)malloc(dhcppkt->optslen);
    memcpy(p, dhcppkt->options, dhcppkt->optslen);


    if(confdata.debug.datablock==true) {
        printf("Datablock size %i:\n", dhcppkt->optslen);
        for(i=0; i<n; i++) printf("%02x", p[i]);
        printf("\nEndDatablock\n");
    }
    
    while(n > 0) {
	code = *p++;
	n--;
	
	if(code == DHCP_OPTION_PAD) continue;
	if(code == DHCP_OPTION_END) break;
	if(n == 0) break;
	
	len = *p++;
	n--;
	if(len > n) break;
	
	switch(code) {
	    case DHCP_OPTION_DHCP_MESSAGE_TYPE: {
		char optstr[MAXLINE];
		logg_msg("DHCP_MESSAGE_TYPE(%d) = %s", code, dhcpmsgtypetostr(p[0],optstr,sizeof(optstr)));
		break;
	    }
	    default: {
		char* data = (char*)malloc((len+1)*2);
		bzero(data, (len+1)*2);
		for(i=0;i<len;i++) snprintf(data+i*2, (len-i+1)*2, "%02x", p[i]);
		char optstr[MAXLINE];
		logg_msg("%s(%d), value=%s", optiontostring(code, optstr, sizeof(optstr)), code, data);
		break;
	    }
	}
	
	p += len;
	n -= len;
    }
    
    logg_msg("----- show ipv4 dhcp packet -----");
}

// функция перевода типа DHCP сообщения в строку описание
//	параметры:
//		code	- номер опции
//		retdata	- указатель на строку, куда будет записано описание
//		retlen	- длина строки
char* dhcpmsgtypetostr(int type, char* retdata, size_t retlen) {
    bzero(retdata, retlen);
    switch(type) {
	case DHCP_DISCOVER	: snprintf(retdata, retlen, "DHCP_DISCOVER"); break;
	case DHCP_OFFER		: snprintf(retdata, retlen, "DHCP_OFFER"); break;
	case DHCP_REQUEST	: snprintf(retdata, retlen, "DHCP_REQUEST"); break;
	case DHCP_DECLINE	: snprintf(retdata, retlen, "DHCP_DECLINE"); break;
	case DHCP_ACK		: snprintf(retdata, retlen, "DHCP_ACK"); break;
	case DHCP_NAK		: snprintf(retdata, retlen, "DHCP_NAK"); break;
	case DHCP_RELEASE	: snprintf(retdata, retlen, "DHCP_RELEASE"); break;
	case DHCP_INFORM	: snprintf(retdata, retlen, "DHCP_INFORM"); break;
	default: snprintf(retdata, retlen, "UNKNOWN_TYPE");
    }
    return retdata;
}


// функция перевода номера DHCP опции в строку-описание
//	параметры:
//		code	- номер опции
//		retdata	- указатель на строку, куда будет записано описание
//		retlen	- длина строки
char* optiontostring(int code, char* retdata, size_t retlen) {
    bzero(retdata, retlen);
    switch(code) {
	case DHCP_OPTION_SUBNET_MASK                 : snprintf(retdata, retlen, "DHCP_OPTION_SUBNET_MASK"); break;
	case DHCP_OPTION_TIME_OFFSET                 : snprintf(retdata, retlen, "DHCP_OPTION_TIME_OFFSET"); break;
	case DHCP_OPTION_ROUTERS                     : snprintf(retdata, retlen, "DHCP_OPTION_ROUTERS"); break;
	case DHCP_OPTION_TIME_SERVERS                : snprintf(retdata, retlen, "DHCP_OPTION_TIME_SERVERS"); break;
	case DHCP_OPTION_NAME_SERVERS                : snprintf(retdata, retlen, "DHCP_OPTION_NAME_SERVERS"); break;
	case DHCP_OPTION_DOMAIN_NAME_SERVERS         : snprintf(retdata, retlen, "DHCP_OPTION_DOMAIN_NAME_SERVERS"); break;
	case DHCP_OPTION_LOG_SERVERS                 : snprintf(retdata, retlen, "DHCP_OPTION_LOG_SERVERS"); break;
	case DHCP_OPTION_COOKIE_SERVERS              : snprintf(retdata, retlen, "DHCP_OPTION_COOKIE_SERVERS"); break;
	case DHCP_OPTION_LPR_SERVERS                 : snprintf(retdata, retlen, "DHCP_OPTION_LPR_SERVERS"); break;
	case DHCP_OPTION_IMPRESS_SERVERS             : snprintf(retdata, retlen, "DHCP_OPTION_IMPRESS_SERVERS"); break;
	case DHCP_OPTION_RESOURCE_LOCATION_SERVERS   : snprintf(retdata, retlen, "DHCP_OPTION_RESOURCE_LOCATION_SERVERS"); break;
	case DHCP_OPTION_HOST_NAME                   : snprintf(retdata, retlen, "DHCP_OPTION_HOST_NAME"); break;
	case DHCP_OPTION_BOOT_SIZE                   : snprintf(retdata, retlen, "DHCP_OPTION_BOOT_SIZE"); break;
	case DHCP_OPTION_MERIT_DUMP                  : snprintf(retdata, retlen, "DHCP_OPTION_MERIT_DUMP"); break;
	case DHCP_OPTION_DOMAIN_NAME                 : snprintf(retdata, retlen, "DHCP_OPTION_DOMAIN_NAME"); break;
	case DHCP_OPTION_SWAP_SERVER                 : snprintf(retdata, retlen, "DHCP_OPTION_SWAP_SERVER"); break;
	case DHCP_OPTION_ROOT_PATH                   : snprintf(retdata, retlen, "DHCP_OPTION_ROOT_PATH"); break;
	case DHCP_OPTION_EXTENSIONS_PATH             : snprintf(retdata, retlen, "DHCP_OPTION_EXTENSIONS_PATH"); break;
	case DHCP_OPTION_IP_FORWARDING               : snprintf(retdata, retlen, "DHCP_OPTION_IP_FORWARDING"); break;
	case DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING    : snprintf(retdata, retlen, "DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING"); break;
	case DHCP_OPTION_POLICY_FILTER               : snprintf(retdata, retlen, "DHCP_OPTION_POLICY_FILTER"); break;
	case DHCP_OPTION_MAX_DGRAM_REASSEMBLY        : snprintf(retdata, retlen, "DHCP_OPTION_MAX_DGRAM_REASSEMBLY"); break;
	case DHCP_OPTION_DEFAULT_IP_TTL              : snprintf(retdata, retlen, "DHCP_OPTION_DEFAULT_IP_TTL"); break;
	case DHCP_OPTION_PATH_MTU_AGING_TIMEOUT      : snprintf(retdata, retlen, "DHCP_OPTION_PATH_MTU_AGING_TIMEOUT"); break;
	case DHCP_OPTION_PATH_MTU_PLATEAU_TABLE      : snprintf(retdata, retlen, "DHCP_OPTION_PATH_MTU_PLATEAU_TABLE"); break;
	case DHCP_OPTION_INTERFACE_MTU               : snprintf(retdata, retlen, "DHCP_OPTION_INTERFACE_MTU"); break;
	case DHCP_OPTION_ALL_SUBNETS_LOCAL           : snprintf(retdata, retlen, "DHCP_OPTION_ALL_SUBNETS_LOCAL"); break;
	case DHCP_OPTION_BROADCAST_ADDRESS           : snprintf(retdata, retlen, "DHCP_OPTION_BROADCAST_ADDRESS"); break;
	case DHCP_OPTION_PERFORM_MASK_DISCOVERY      : snprintf(retdata, retlen, "DHCP_OPTION_PERFORM_MASK_DISCOVERY"); break;
	case DHCP_OPTION_MASK_SUPPLIER               : snprintf(retdata, retlen, "DHCP_OPTION_MASK_SUPPLIER"); break;
	case DHCP_OPTION_ROUTER_DISCOVERY            : snprintf(retdata, retlen, "DHCP_OPTION_ROUTER_DISCOVERY"); break;
	case DHCP_OPTION_ROUTER_SOLICITATION_ADDRESS : snprintf(retdata, retlen, "DHCP_OPTION_ROUTER_SOLICITATION_ADDRESS"); break;
	case DHCP_OPTION_STATIC_ROUTES               : snprintf(retdata, retlen, "DHCP_OPTION_STATIC_ROUTES"); break;
	case DHCP_OPTION_TRAILER_ENCAPSULATION       : snprintf(retdata, retlen, "DHCP_OPTION_TRAILER_ENCAPSULATION"); break;
	case DHCP_OPTION_ARP_CACHE_TIMEOUT           : snprintf(retdata, retlen, "DHCP_OPTION_ARP_CACHE_TIMEOUT"); break;
	case DHCP_OPTION_IEEE802_3_ENCAPSULATION     : snprintf(retdata, retlen, "DHCP_OPTION_IEEE802_3_ENCAPSULATION"); break;
	case DHCP_OPTION_DEFAULT_TCP_TTL             : snprintf(retdata, retlen, "DHCP_OPTION_DEFAULT_TCP_TTL"); break;
	case DHCP_OPTION_TCP_KEEPALIVE_INTERVAL      : snprintf(retdata, retlen, "DHCP_OPTION_TCP_KEEPALIVE_INTERVAL"); break;
	case DHCP_OPTION_TCP_KEEPALIVE_GARBAGE       : snprintf(retdata, retlen, "DHCP_OPTION_TCP_KEEPALIVE_GARBAGE"); break;
	case DHCP_OPTION_NIS_DOMAIN                  : snprintf(retdata, retlen, "DHCP_OPTION_NIS_DOMAIN"); break;
	case DHCP_OPTION_NIS_SERVERS                 : snprintf(retdata, retlen, "DHCP_OPTION_NIS_SERVERS"); break;
	case DHCP_OPTION_NTP_SERVERS                 : snprintf(retdata, retlen, "DHCP_OPTION_NTP_SERVERS"); break;
	case DHCP_OPTION_VENDOR_ENCAPSULATED_OPTIONS : snprintf(retdata, retlen, "DHCP_OPTION_VENDOR_ENCAPSULATED_OPTIONS"); break;
	case DHCP_OPTION_NETBIOS_NAME_SERVERS        : snprintf(retdata, retlen, "DHCP_OPTION_NETBIOS_NAME_SERVERS"); break;
	case DHCP_OPTION_NETBIOS_DD_SERVER           : snprintf(retdata, retlen, "DHCP_OPTION_NETBIOS_DD_SERVER"); break;
	case DHCP_OPTION_NETBIOS_NODE_TYPE           : snprintf(retdata, retlen, "DHCP_OPTION_NETBIOS_NODE_TYPE"); break;
	case DHCP_OPTION_NETBIOS_SCOPE               : snprintf(retdata, retlen, "DHCP_OPTION_NETBIOS_SCOPE"); break;
	case DHCP_OPTION_FONT_SERVERS                : snprintf(retdata, retlen, "DHCP_OPTION_FONT_SERVERS"); break;
	case DHCP_OPTION_X_DISPLAY_MANAGER           : snprintf(retdata, retlen, "DHCP_OPTION_X_DISPLAY_MANAGER"); break;
	case DHCP_OPTION_DHCP_REQUESTED_ADDRESS      : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_REQUESTED_ADDRESS"); break;
	case DHCP_OPTION_DHCP_LEASE_TIME             : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_LEASE_TIME"); break;
	case DHCP_OPTION_DHCP_OPTION_OVERLOAD        : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_OPTION_OVERLOAD"); break;
	case DHCP_OPTION_DHCP_MESSAGE_TYPE           : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_MESSAGE_TYPE"); break;
	case DHCP_OPTION_DHCP_SERVER_IDENTIFIER      : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_SERVER_IDENTIFIER"); break;
	case DHCP_OPTION_DHCP_PARAMETER_REQUEST_LIST : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_PARAMETER_REQUEST_LIST"); break;
	case DHCP_OPTION_DHCP_MESSAGE                : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_MESSAGE"); break;
	case DHCP_OPTION_DHCP_MAX_MESSAGE_SIZE       : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_MAX_MESSAGE_SIZE"); break;
	case DHCP_OPTION_DHCP_RENEWAL_TIME           : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_RENEWAL_TIME"); break;
	case DHCP_OPTION_DHCP_REBINDING_TIME         : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_REBINDING_TIME"); break;
	case DHCP_OPTION_VENDOR_CLASS_IDENTIFIER     : snprintf(retdata, retlen, "DHCP_OPTION_VENDOR_CLASS_IDENTIFIER"); break;
	case DHCP_OPTION_DHCP_CLIENT_IDENTIFIER      : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_CLIENT_IDENTIFIER"); break;
	case DHCP_OPTION_NWIP_DOMAIN_NAME            : snprintf(retdata, retlen, "DHCP_OPTION_NWIP_DOMAIN_NAME"); break;
	case DHCP_OPTION_NWIP_SUBOPTIONS             : snprintf(retdata, retlen, "DHCP_OPTION_NWIP_SUBOPTIONS"); break;
	case DHCP_OPTION_FTP_SERVER                  : snprintf(retdata, retlen, "DHCP_OPTION_FTP_SERVER"); break;
	case DHCP_OPTION_BOOT_FILE                   : snprintf(retdata, retlen, "DHCP_OPTION_BOOT_FILE"); break;
	case DHCP_OPTION_USER_CLASS                  : snprintf(retdata, retlen, "DHCP_OPTION_USER_CLASS"); break;
	case DHCP_OPTION_FQDN                        : snprintf(retdata, retlen, "DHCP_OPTION_FQDN"); break;
	case DHCP_OPTION_DHCP_AGENT_OPTIONS          : snprintf(retdata, retlen, "DHCP_OPTION_DHCP_AGENT_OPTIONS"); break;

	default: snprintf(retdata, retlen, "UNKNOWN_CODE_%i", code);
    }
    
    return retdata;
}

/// -------------------- функция добавления опций в лист по типу и коду -------------------------
dhcpv4_options_list*	dhcpv4_addoption_long(dhcpv4_options_list *list, const int code, const unsigned long data) {
    dhcpv4_option	option;

    option.code		= code;
    option.len		= 4;
    option.data		= (uchar*)malloc(option.len);
    hnputl(option.data, data);
    
    list = dhcpv4_addoption(list, option);
    return list;
}

dhcpv4_options_list*	dhcpv4_addoption_iplist2(dhcpv4_options_list *list, const int code, const char* ips) {
    dhcpv4_option	option;
    char		arg_ip[IPV4_STR_LEN];
    struct in_addr	in_ip;
    int			ip_count = 0;
    uchar		data[MAXLINE];

    bzero(data, sizeof(data));
    option.code		= code;
    
    bzero(&arg_ip, sizeof(arg_ip));
    
    int n = 0;
    int p = 0;
    
//    printf("add= %s\n", ips);
//    printf("strlen=%i\n", strlen(ips));
    
    int len = strlen(ips);

    while(n <= len) {
	if(ip_count*4 > sizeof(data)) break;
	n++;
	
	char	c = *ips++;
	
	
	if((c!=(char)32)&&(c!=(char)0)) {
	    arg_ip[p++] = c;
	} else {
	    bzero(&in_ip, sizeof(in_ip));
	    inet_aton(arg_ip, &in_ip);
	    hnputl2(data+(ip_count*4), in_ip.s_addr);
	    ip_count++;
	    
	    p = 0;
	    bzero(&arg_ip, sizeof(arg_ip));
	};
    }

    option.len 		= (ip_count)*4;
    option.data		= (uchar*)malloc(option.len);
    memcpy(option.data, data, option.len);

    list = dhcpv4_addoption(list, option);
    return list;
}


dhcpv4_options_list*	dhcpv4_addoption_iplist(dhcpv4_options_list *list, const int code, ...) {
    dhcpv4_option	option;
    va_list		ap;
    uchar		data[MAXLINE];
    char*		arg_ip;
    struct in_addr	in_ip;
    int			ip_count = 0;

    bzero(data, sizeof(data));
    option.code		= code;

    va_start(ap, code);
    while((arg_ip = va_arg(ap, char*)) != NULL) {
	if(ip_count*4 > sizeof(data)) break;
	
	bzero(&in_ip, sizeof(in_ip));
	inet_aton(arg_ip, &in_ip);
	hnputl2(data+(ip_count*4), in_ip.s_addr);
	ip_count++;
    }
    va_end(ap);

    option.len 		= (ip_count-1)*4;
    option.data		= (uchar*)malloc(option.len);
    memcpy(option.data, data, option.len);

    list = dhcpv4_addoption(list, option);
    return list;
}

dhcpv4_options_list*	dhcpv4_addoption_ip(dhcpv4_options_list *list, const int code, const char* ip) {
    dhcpv4_option	option;

    struct in_addr	in_ip;
    inet_aton(ip, &in_ip);
    
    option.code		= code;
    option.len		= 4;
    option.data		= (uchar*)malloc(option.len);
    hnputl2(option.data, in_ip.s_addr);
    
    list = dhcpv4_addoption(list, option);
    return list;
}

dhcpv4_options_list*	dhcpv4_addoption_string(dhcpv4_options_list* list, const int code, const char* string) {
    dhcpv4_option	option;
    
    option.code		= code;
    option.len		= strlen(string);
    option.data		= (uchar*)malloc(option.len);
    memcpy(option.data, string, strlen(string));
    
    list = dhcpv4_addoption(list, option);
    return list;
}

dhcpv4_options_list*	dhcpv4_addoption_binary(dhcpv4_options_list* list, const int code, const uchar* data, const size_t datalen) {
    dhcpv4_option	option;
    
    option.code		= code;
    option.len		= datalen;
    option.data		= (uchar*)malloc(option.len);
    memcpy(option.data, data, datalen);
    
    list = dhcpv4_addoption(list, option);
    return list;
}

dhcpv4_options_list*	dhcpv4_addoption_byte(dhcpv4_options_list* list, const int code, const int data) {
    dhcpv4_option	option;
    
    option.code		= code;
    option.len		= 1;
    option.data		= (uchar*)malloc(option.len);
    option.data[0]	= data;
    
    list = dhcpv4_addoption(list, option);
    return list;
}

/// -------------------- функции процессинга входящих сообщений по типам ----------------------------

// Процессинг сообщения DHCP_DISCOVER
//   параметры: 
//	pkt  - разобраный пакет-запрос
//	opts - список опций
//	iIndex	- номер интерфейса, с которого пришёл запрос
dhcpv4_raw_packet* dhcpv4_process_discover(dhcpv4_pkt* pkt, dhcpv4_options_list* opts, int iIndex) {
    dhcpv4_raw_packet*		raw = NULL;	// возвращаемый "сырой" пакет
    dhcpv4_pkt			rpkt;		// возвращаемый dhcp пакет
    dhcpv4_options_list*	ropts = NULL;	// список опций для отправки клиенту
    
    char	cl_addr[IPV4_STR_LEN];
    bzero(&cl_addr, sizeof(cl_addr));
    
    char	chaddr[DHCP_CHADDR_LEN*2]; bzero(&chaddr, sizeof(chaddr));
    char	cmac[DHCP_CHADDR_LEN*2]; bzero(&cmac, sizeof(cmac));
    snprintf(cmac, sizeof(cmac), "%s", convert_chaddrtostring(pkt, chaddr, sizeof(chaddr)));
    
    if(!dhcpv4_discover_ip(iIndex, cl_addr, sizeof(cl_addr), cmac)) {
	return NULL;
    }
    
    struct in_addr	client_ip;			// ip адрес, предлагаемый клиенту
    inet_aton(cl_addr, &client_ip);

    struct in_addr	server_ip;			// ip адрес сервера
    inet_aton(confdata.server_ip, &server_ip);
    
    bzero(&rpkt, sizeof(dhcpv4_pkt));
    
    rpkt.op		= DHCP_BOOTREPLY;
    rpkt.htype		= pkt->htype;
    rpkt.hlen		= pkt->hlen;
    rpkt.hops		= 0;
    
    rpkt.xid		= pkt->xid;
    
    rpkt.secs		= 0;
    rpkt.flags		= pkt->flags;

    inet_aton("0.0.0.0", &rpkt.ciaddr);
    rpkt.yiaddr		= client_ip;
    rpkt.siaddr		= server_ip;
    inet_aton("0.0.0.0", &rpkt.giaddr);
    
    memcpy(rpkt.chaddr, pkt->chaddr, sizeof(rpkt.chaddr));
    memcpy(rpkt.sname, pkt->sname, sizeof(rpkt.sname));
    memcpy(rpkt.file, pkt->file, sizeof(rpkt.file));
    
    memcpy(rpkt.magic, optmagic, sizeof(rpkt.magic));
    
    // заполним опции
    ropts = dhcpv4_addoption_byte(ropts,	DHCP_OPTION_DHCP_MESSAGE_TYPE, DHCP_OFFER);
    ropts = dhcpv4_db_options(cl_addr, ropts);
    ropts = dhcpv4_addoption_ip(ropts,		DHCP_OPTION_DHCP_SERVER_IDENTIFIER, confdata.server_ip);

/*    ropts = dhcpv4_addoption_iplist(ropts,	DHCP_OPTION_DOMAIN_NAME_SERVERS, "192.166.12.2", "192.166.15.2");
    ropts = dhcpv4_addoption_ip(ropts, 		DHCP_OPTION_SUBNET_MASK, "255.255.255.0");
    ropts = dhcpv4_addoption_string(ropts, 	DHCP_OPTION_DOMAIN_NAME, "ipoeclients.luglink.net");
    ropts = dhcpv4_addoption_ip(ropts, 		DHCP_OPTION_ROUTERS, "10.255.12.254");
    ropts = dhcpv4_addoption_long(ropts,	DHCP_OPTION_DHCP_LEASE_TIME, 60);			// 60 sec
    ropts = dhcpv4_addoption_ip(ropts,		DHCP_OPTION_DHCP_SERVER_IDENTIFIER, confdata.server_ip);
*/
    raw = dhcpv4_generate_rawpkt(&rpkt, ropts);
    
    free(ropts);
    
    return raw;
}

dhcpv4_raw_packet* dhcpv4_process_request(dhcpv4_pkt* pkt, dhcpv4_options_list* opts, int iIndex) {
    dhcpv4_raw_packet*		raw = NULL;	// возвращаемый "сырой" пакет
    dhcpv4_pkt			rpkt;		// возвращаемый dhcp пакет
    dhcpv4_options_list*	ropts = NULL;	// список опций для отправки клиенту
    
    char	cl_addr[IPV4_STR_LEN];
    bzero(&cl_addr, sizeof(cl_addr));

    struct in_addr in;
    char	chaddr[DHCP_CHADDR_LEN*2]; bzero(&chaddr, sizeof(chaddr));
    char	cmac[DHCP_CHADDR_LEN*2]; bzero(&cmac, sizeof(cmac));

    int db_req_result = DHCP_NAK;

    dhcpv4_option* req_opt = NULL;
    req_opt = dhcpv4_find_option(opts, DHCP_OPTION_DHCP_REQUESTED_ADDRESS);
    
    if(req_opt == NULL) {
	logg_msg("process request: DHCP_OPTION_DHCP_REQUESTED_ADDRESS not found");
//	mydhcp_show_ipv4_pkt(pkt);
//	return NULL;
	in.s_addr = pkt->ciaddr.s_addr;
	snprintf(cl_addr, sizeof(cl_addr), "%s", inet_ntoa(in));
    } else {
	in.s_addr = nhgetl2(req_opt->data);
	snprintf(cl_addr, sizeof(cl_addr), "%s", inet_ntoa(in));
    }

    snprintf(cmac, sizeof(cmac), "%s", convert_chaddrtostring(pkt, chaddr, sizeof(chaddr)));
    db_req_result = dhcpv4_request_ip(iIndex, cl_addr, cmac);
    
    
    struct in_addr	client_ip;			// ip адрес, предлагаемый клиенту
    inet_aton(cl_addr, &client_ip);

    bzero(&rpkt, sizeof(dhcpv4_pkt));
    
    rpkt.op		= DHCP_BOOTREPLY;
    rpkt.htype		= pkt->htype;
    rpkt.hlen		= pkt->hlen;
    rpkt.hops		= 0;
    
    rpkt.xid		= pkt->xid;
    
    rpkt.secs		= 0;
    rpkt.flags		= pkt->flags;

    inet_aton("0.0.0.0", &rpkt.ciaddr);
    rpkt.yiaddr		= client_ip;
    inet_aton("0.0.0.0", &rpkt.siaddr);
    inet_aton("0.0.0.0", &rpkt.giaddr);
    
    memcpy(rpkt.chaddr, pkt->chaddr, sizeof(rpkt.chaddr));
    memcpy(rpkt.sname, pkt->sname, sizeof(rpkt.sname));
    memcpy(rpkt.file, pkt->file, sizeof(rpkt.file));
    
    memcpy(rpkt.magic, optmagic, sizeof(rpkt.magic));
    
    // заполним опции
    ropts = dhcpv4_addoption_byte(ropts,	DHCP_OPTION_DHCP_MESSAGE_TYPE, db_req_result);
    if(db_req_result == DHCP_ACK) ropts = dhcpv4_db_options(cl_addr, ropts);
    ropts = dhcpv4_addoption_ip(ropts,		DHCP_OPTION_DHCP_SERVER_IDENTIFIER, confdata.server_ip);

/*    ropts = dhcpv4_addoption_iplist(ropts,	DHCP_OPTION_DOMAIN_NAME_SERVERS, "192.166.12.2", "192.166.15.2");
    ropts = dhcpv4_addoption_ip(ropts, 		DHCP_OPTION_SUBNET_MASK, "255.255.255.0");
    ropts = dhcpv4_addoption_string(ropts, 	DHCP_OPTION_DOMAIN_NAME, "ipoeclients.luglink.net");
    ropts = dhcpv4_addoption_ip(ropts, 		DHCP_OPTION_ROUTERS, "10.255.12.254");
    ropts = dhcpv4_addoption_long(ropts,	DHCP_OPTION_DHCP_LEASE_TIME, 60);			// 60 sec
    ropts = dhcpv4_addoption_ip(ropts,		DHCP_OPTION_DHCP_SERVER_IDENTIFIER, confdata.server_ip);
*/
    raw = dhcpv4_generate_rawpkt(&rpkt, ropts);
    
    free(ropts);
    
    if((db_req_result==DHCP_ACK)&&(confdata.exec_external==true)) {
        pid_t epid;
        
        epid = fork();
        if(epid>0) return raw;
        if(epid==0) {
	    char	siIndex[MAXLINE];
	    interface_name(iIndex, siIndex);
            execl(confdata.external, confdata.external, cl_addr, siIndex, (char *)NULL);
            _exit(3);
    	}
    }
    
    return raw;
}