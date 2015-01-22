#include <time.h>
#include "logg.h"

static void logg_push(int, int, const char*, va_list);

// Информативный лог
void logg_msg(const char* fmt, ...) {
    va_list	ap;
    
    va_start(ap, fmt);
    logg_push(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

// Лог и выход из процесса
void logg_quit(const char* fmt, ...) {
    va_list	ap;
    
    va_start(ap, fmt);
    logg_push(0, LOG_ERR, fmt, ap);
    va_end(ap);
    
    _exit(3);
}

// Вывод информации
static void logg_push(int errnoflag, int level, const char* fmt, va_list ap) {
    int		save_errno, n;
    char	buf[MAXLINE];
    time_t	t = time(NULL);
    char	bufTime[MAXLINE];
    
    if(confdata.debug.info!=true) return;
    
    save_errno = errno;
    
    vsnprintf(buf, sizeof(buf), fmt, ap);
    n = strlen(buf);
    
    if(errnoflag)
	snprintf(buf+n, sizeof(buf)-n, ": %s", strerror(save_errno));

    if(confdata.daemon==false) {
	strftime(bufTime, sizeof(bufTime), "[%d/%b/%Y %H:%M:%S] ", localtime(&t));
	n = strlen(bufTime);
	snprintf(bufTime+n, sizeof(bufTime)-n, " %s", buf);
	memcpy(buf, bufTime, sizeof(buf));
    } 

    strcat(buf, "\n");
    
    if(confdata.daemon==true) {
	syslog(level, "%s", buf);
    } else {
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
    };
    
    return;
}
