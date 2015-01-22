#include <signal.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>
#include "defines.h"
#include "main.h"
#include "logg.h"
#include "listener.h"
#include "dhcp_db.h"
#include "mydhcp.h"
#include "dhcp_route.h"

#include <confuse.h>

static char config_file[MAXLINE];

static void process_args(int argc, char** argv);
static void show_info();

void daemonize();

void signal_SIGCHLD(int sig_num);

int main(int argc, char** argv) {
    int sockfd;
    int SERV_PORT = 67;
    
    
    bzero(&config_file, sizeof(config_file));
    snprintf(config_file, sizeof(config_file), "%s", DEFAULT_CONFIG);
    
    if(argc > 1) process_args(argc, argv);
    
    cfg_opt_t opts[] = {
	CFG_BOOL("DAEMON", true, CFGF_NONE),

	CFG_STR("SERVER_IP", "127.0.0.1", CFGF_NONE),
	CFG_INT("NAS_ID", 1, CFGF_NONE),
	CFG_INT("OFFER_LEASE_TIME", 10, CFGF_NONE),
	CFG_INT("DEFAULT_LEASE_TIME", 600, CFGF_NONE),
	CFG_INT("MINIMAL_UDP_PKT_SIZE", DHCP_MIN_PKT_LEN, CFGF_NONE),
	
	CFG_BOOL("EXEC_EXTERNAL", false, CFGF_NONE),
	CFG_STR("EXTERNAL", "", CFGF_NONE),

	CFG_STR("DB_SERVER", "localhost", CFGF_NONE),
	CFG_STR("DB_USER", "root", CFGF_NONE),
	CFG_STR("DB_PASSWORD", "", CFGF_NONE),
	CFG_STR("DB_NAME", "dhcp", CFGF_NONE),
	
	CFG_BOOL("DEBUG_INFO", true, CFGF_NONE),
	CFG_BOOL("DEBUG_IN_PKT", false, CFGF_NONE),
	CFG_BOOL("DEBUG_DATABLOCK", false, CFGF_NONE),
	CFG_END()
    };
    cfg_t* cfg;
    
    cfg = cfg_init(opts, CFGF_NONE);
    if(cfg_parse(cfg, config_file) != CFG_SUCCESS) {
	confdata.debug.info = true;
        logg_quit("Ошибка работы с файлом конфигурации");
    }
    
    confdata.daemon = cfg_getbool(cfg, "DAEMON");
    
    snprintf(confdata.server_ip, sizeof(confdata.server_ip), "%s", cfg_getstr(cfg, "SERVER_IP"));
    confdata.nasid = cfg_getint(cfg, "NAS_ID");
    confdata.offer_lease_time = cfg_getint(cfg, "OFFER_LEASE_TIME");
    confdata.default_lease_time = cfg_getint(cfg, "DEFAULT_LEASE_TIME");
    confdata.min_udp_size = cfg_getint(cfg, "MINIMAL_UDP_PKT_SIZE");
    
    confdata.exec_external = cfg_getbool(cfg, "EXEC_EXTERNAL");
    snprintf(confdata.external, sizeof(confdata.external), "%s", cfg_getstr(cfg, "EXTERNAL"));
    
    // Параметры подключения к БД
    snprintf(confdata.db_server, sizeof(confdata.db_server), "%s", cfg_getstr(cfg, "DB_SERVER"));
    snprintf(confdata.db_user, sizeof(confdata.db_user), "%s", cfg_getstr(cfg, "DB_USER"));
    snprintf(confdata.db_password, sizeof(confdata.db_password), "%s", cfg_getstr(cfg, "DB_PASSWORD"));
    snprintf(confdata.db_name, sizeof(confdata.db_name), "%s", cfg_getstr(cfg, "DB_NAME"));
    
    // Праметры DEBUG
    confdata.debug.info		= cfg_getbool(cfg, "DEBUG_INFO");
    confdata.debug.in_pkt	= cfg_getbool(cfg, "DEBUG_IN_PKT");
    confdata.debug.datablock	= cfg_getbool(cfg, "DEBUG_DATABLOCK");
    
    cfg_free(cfg);

    // Инициализация подключений
    dhcp_db_init();
    dhcp_rtv4_init();
    
//    dhcp_rtv4_findroute("10.255.12.1", 32);
    
//    exit(0);
    
    // Очистим устаревшие аренды
    dhcpv4_db_clear_leases();

    struct sockaddr_in	servaddr, cliaddr;
    
    if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	logg_quit("socket error %i: %s", errno, strerror(errno));
    };
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family		= AF_INET;
    servaddr.sin_addr.s_addr	= htonl(INADDR_ANY);
    servaddr.sin_port		= htons(SERV_PORT);
    
    if(bind(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) logg_quit("bind error %i: %s", errno, strerror(errno));
    
//    signal(SIGCHLD, signal_SIGCHLD);
    signal(SIGCHLD, SIG_IGN);
    
    if(confdata.daemon==true) daemonize();
    
    listener(sockfd, (SA *)&cliaddr, sizeof(cliaddr));
    return 0;
}

void process_args(int argc, char** argv) {
    int i;
    for (i = 1; i < argc; i++) {
	if(!strcmp(argv[i], "--help")) show_info();
	if(!strcmp(argv[i], "-c")) {
            if(argc < 3) show_info();

            bzero(&config_file, sizeof(config_file));
            snprintf(config_file, sizeof(config_file), "%s", argv[i+1]);
	}
    }
}

void show_info() {
    printf("MoonDHCP версии %s\n", MD_VERSION);
    printf("Параметры запуска:\n");
    printf("\t--help\t\tданная справка\n");
    printf("\t-c <file.conf>\tиспользовать файл конфигурации file.conf.\n");
    printf("\t\t\tПо умолчанию используется %s\n", DEFAULT_CONFIG);
    _exit(3);
}

void daemonize() {
    pid_t pid;
    if((pid=fork())!=0) _exit(3);
    
    setsid();
    
    signal(SIGHUP, SIG_IGN);
    if((pid=fork())!=0) _exit(3);
    
    openlog("MoonDHCP", LOG_PID, LOG_DAEMON);
}

void signal_SIGCHLD(int sig_num) {
    int result;
    
    wait(&result);
}

