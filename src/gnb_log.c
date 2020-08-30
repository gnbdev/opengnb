#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#endif


#include "gnb_log.h"
#include "gnb_time.h"
#include "gnb_payload16.h"


#define GNB_LOG_LINE_MAX 1024*4

static void open_log_file(gnb_log_ctx_t *log_ctx){

	log_ctx->std_fd   = open(log_ctx->log_file_name_std,   O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	log_ctx->debug_fd = open(log_ctx->log_file_name_debug, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	log_ctx->error_fd = open(log_ctx->log_file_name_error, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);

}


static void close_log_file_pre_fd(gnb_log_ctx_t *log_ctx){

	if ( log_ctx->pre_std_fd > 0 ){
		close(log_ctx->pre_std_fd);
		log_ctx->pre_std_fd = -1;
	}

	if ( log_ctx->pre_debug_fd > 0 ){
		close(log_ctx->pre_debug_fd);
		log_ctx->pre_debug_fd = -1;
	}

	if ( log_ctx->pre_error_fd > 0 ){
		close(log_ctx->pre_error_fd);
		log_ctx->pre_error_fd = -1;
	}

}


static void log_file_output(gnb_log_ctx_t *log_ctx, uint8_t log_type, char *log_string, int log_string_len){

    int ret;

    switch (log_type) {

        case GNB_LOG_TYPE_STD:
            ret = write(log_ctx->std_fd, log_string, log_string_len);
            break;

        case GNB_LOG_TYPE_DEBUG:
        	ret = write(log_ctx->debug_fd, log_string, log_string_len);
            break;

        case GNB_LOG_TYPE_ERROR:
        	ret = write(log_ctx->error_fd, log_string, log_string_len);
            break;

        default:
            break;

    }

}


static void log_console_output(int log_type, char *log_string, int log_string_len){

	int rc;

    switch (log_type) {

        case GNB_LOG_TYPE_STD:
        case GNB_LOG_TYPE_DEBUG:
            rc = write(STDOUT_FILENO, log_string, log_string_len);
            break;

        case GNB_LOG_TYPE_ERROR:
        	rc = write(STDERR_FILENO, log_string, log_string_len);
            break;

        default:
            break;

    }

}


static void log_udp_output(gnb_log_ctx_t *log_ctx, uint8_t log_type, char *log_string, int log_string_len){

	struct sockaddr_in6 saddr6;
	struct sockaddr_in  saddr4;

    if( log_ctx->socket6_fd >0 ){

        memset(&saddr6,0, sizeof(struct sockaddr_in6));
        saddr6.sin6_family = AF_INET6;
        saddr6.sin6_port   = log_ctx->port6;
        memcpy(&saddr6.sin6_addr, log_ctx->addr6, 16);

    	sendto(log_ctx->socket6_fd, log_string,
    			log_string_len, 0,
    		    (struct sockaddr *)&saddr6, sizeof(struct sockaddr_in6));

    }

    if( log_ctx->socket4_fd >0 ){

        memset(&saddr4,0, sizeof(struct sockaddr_in));
        saddr4.sin_family = AF_INET;
        saddr4.sin_port   = log_ctx->port4;

        memcpy(&saddr4.sin_addr.s_addr, log_ctx->addr4, 4);

    	sendto(log_ctx->socket4_fd, log_string,
    			log_string_len, 0,
    		   (struct sockaddr *)&saddr4, sizeof(struct sockaddr_in));

    }

}



static void log_udp_binary_output(gnb_log_ctx_t *log_ctx, uint8_t log_type, uint8_t log_id, char *log_string_buffer, int log_string_len){

	struct sockaddr_in6 saddr6;
	struct sockaddr_in saddr4;

	uint16_t data_size = (uint16_t)log_string_len;

	gnb_payload16_t *gnb_payload16 = (gnb_payload16_t *)log_string_buffer;

	gnb_payload16->type     = log_ctx->log_payload_type;

	gnb_payload16->sub_type = log_id;

	gnb_payload16_set_data_len(gnb_payload16, data_size);

    if( log_ctx->socket6_fd >0 ){

        memset(&saddr6,0, sizeof(struct sockaddr_in));
        saddr6.sin6_family = AF_INET6;
        saddr6.sin6_port   = log_ctx->port6;
        memcpy(&saddr6.sin6_addr, log_ctx->addr6, 16);

    	sendto(log_ctx->socket6_fd, (void *)gnb_payload16,
    			data_size+4, 0,
    		    (struct sockaddr *)&saddr6, sizeof(struct sockaddr_in6));

    }

    if( log_ctx->socket4_fd >0 ){

        memset(&saddr4,0, sizeof(struct sockaddr_in));
        saddr4.sin_family = AF_INET;
        saddr4.sin_port   = log_ctx->port4;

        memcpy(&saddr4.sin_addr.s_addr, log_ctx->addr4, 4);

    	sendto(log_ctx->socket4_fd, (void *)gnb_payload16,
    			data_size+4, 0,
    		   (struct sockaddr *)&saddr4, sizeof(struct sockaddr_in));

    }

}


void gnb_logf(gnb_log_ctx_t *log_ctx, uint8_t log_type, uint8_t log_id, uint8_t level, const char *format, ...){

	char now_time_string[GNB_TIME_STRING_MAX];

	char log_string_buffer[GNB_LOG_LINE_MAX+4];

	char *log_string;

	int log_string_len;
	int len;

	char *p;

	//加上 一个 offset 4 用于后面的 udp binary output 可以加上一个4字节的 gnb_payload 首部
	log_string = log_string_buffer+4;

	p = log_string;

	gnb_now_timef("%y-%m-%d %H:%M:%S", now_time_string, GNB_TIME_STRING_MAX);

	len = snprintf(p, GNB_LOG_LINE_MAX, "%s %s ", now_time_string, log_ctx->config_table[log_id].log_name);

	log_string_len = len;

	if ( log_string_len > GNB_LOG_LINE_MAX ){
		return;
	}

	p += log_string_len;

	va_list ap;

	va_start(ap, format);

	len = vsnprintf(p, GNB_LOG_LINE_MAX, format, ap);

	va_end(ap);

	log_string_len += len;

	if ( log_string_len > GNB_LOG_LINE_MAX ){
		return;
	}

	if ( log_ctx->output_type & GNB_LOG_OUTPUT_STDOUT ){
		log_console_output(log_type, log_string,log_string_len);
	}

	if ( log_ctx->output_type & GNB_LOG_OUTPUT_FILE ){
		log_file_output(log_ctx, log_type, log_string, log_string_len);
	}

	if ( log_ctx->output_type & GNB_LOG_OUTPUT_UDP ){

		if (GNB_LOG_UDP_TYPE_BINARY == log_ctx->log_udp_type){
			log_udp_binary_output(log_ctx, log_type, log_id, log_string_buffer, log_string_len);
		}else{
			log_udp_output(log_ctx, log_type, log_string, log_string_len);
		}

	}

}


int gnb_log_udp_open(gnb_log_ctx_t *log_ctx){

	struct sockaddr_in6 svr_addr6;
	struct sockaddr_in svr_addr;

	int on;

	int ret;

	log_ctx->socket6_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    on = 1;
	setsockopt(log_ctx->socket6_fd, SOL_SOCKET, SO_REUSEADDR,(char *)&on, sizeof(on));
	on = 1;
	setsockopt(log_ctx->socket6_fd, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on));
    memset(&svr_addr6,0, sizeof(struct sockaddr_in6));
    svr_addr6.sin6_family = AF_INET6;
    svr_addr6.sin6_port   = 0;
    svr_addr6.sin6_addr   = in6addr_any;
    ret = bind(log_ctx->socket6_fd, (struct sockaddr *)&svr_addr6, sizeof(struct sockaddr_in6));


	log_ctx->socket4_fd = socket(AF_INET,  SOCK_DGRAM, 0);
	on = 1;
	setsockopt(log_ctx->socket4_fd, SOL_SOCKET, SO_REUSEADDR,(char *)&on, sizeof(on) );
    memset(&svr_addr,0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = 0;
    svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(log_ctx->socket4_fd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in));

    memcpy(log_ctx->addr6,  &in6addr_loopback, 16);
    log_ctx->port6 = htons(9000);

    *((uint32_t *)log_ctx->addr4) = htonl(INADDR_LOOPBACK);
    log_ctx->port4 = htons(9000);

	return 0;

}


int gnb_log_udp_set_addr4(gnb_log_ctx_t *log_ctx, char *ip, uint16_t port4){

	inet_pton(AF_INET, ip, (struct in_addr *)log_ctx->addr4);

	log_ctx->port4 = htons(port4);

	return 0;
}


int gnb_log_udp_set_addr6(gnb_log_ctx_t *log_ctx, char *ip, uint16_t port6){

	inet_pton(AF_INET6, ip, (struct in6_addr *)log_ctx->addr6);

	log_ctx->port6 = htons(port6);

	return 0;
}


int gnb_log_udp_set_addr4_string(gnb_log_ctx_t *log_ctx, char *sockaddress4_string){

	unsigned long int ul;

	#define MAX_SOCKADDRESS_STRING ( 16 + 1 + sizeof("65535") )

	char sockaddress4_string_copy[MAX_SOCKADDRESS_STRING+1];

	int sockaddress4_string_len = strlen(sockaddress4_string);

	if(sockaddress4_string_len > MAX_SOCKADDRESS_STRING ){
		return -1;
	}

	strncpy(sockaddress4_string_copy, sockaddress4_string, MAX_SOCKADDRESS_STRING);

	int i;

	char *p = sockaddress4_string_copy;

	for( i=0; i<sockaddress4_string_len; i++){

		if ( ':' == *p ){
			*p = '\0';
			break;
		}

		p++;

	}

	p++;

	ul = strtoul(p, NULL, 10);

	if( ULONG_MAX == ul ){
    	return -1;
	}

	uint16_t port = (uint16_t)ul;
	log_ctx->port4 = htons(port);

	inet_pton(AF_INET, sockaddress4_string_copy, (struct in_addr *)log_ctx->addr4);

	return 0;

}


gnb_log_ctx_t* gnb_log_ctx_create(){

	gnb_log_ctx_t *log_ctx = (gnb_log_ctx_t *)malloc(sizeof(gnb_log_ctx_t));

	memset(log_ctx, 0, sizeof(gnb_log_ctx_t));

	log_ctx->std_fd   = -1;
	log_ctx->debug_fd = -1;
	log_ctx->error_fd = -1;

	log_ctx->pre_std_fd    = -1;
	log_ctx->pre_debug_fd  = -1;
	log_ctx->pre_error_fd  = -1;

	log_ctx->socket6_fd    = -1;
	log_ctx->socket4_fd    = -1;

	return log_ctx;

}


int gnb_log_file_rotate(gnb_log_ctx_t *log_ctx){

    char now_time_string[GNB_TIME_STRING_MAX];

    char archive_name[PATH_MAX+NAME_MAX];

	int mday = gnb_now_mday();

	if (mday == log_ctx->pre_mday) {
		close_log_file_pre_fd(log_ctx);
		return 0;
	}

	log_ctx->pre_mday = mday;

	gnb_now_timef("%Y_%m_%d",now_time_string,GNB_TIME_STRING_MAX);

	snprintf(archive_name, PATH_MAX+NAME_MAX, "%s/std_%s.log.arc", log_ctx->log_file_path, now_time_string);
	rename( (const char *)log_ctx->log_file_name_std, (const char *)archive_name );
	log_ctx->pre_std_fd    = log_ctx->std_fd;

	log_ctx->pre_debug_fd  = log_ctx->debug_fd;
	snprintf(archive_name, PATH_MAX+NAME_MAX, "%s/debug_%s.log.arc", log_ctx->log_file_path, now_time_string);
	rename( (const char *)log_ctx->log_file_name_debug, (const char *)archive_name );


	log_ctx->pre_error_fd  = log_ctx->error_fd;
	snprintf(archive_name, PATH_MAX+NAME_MAX, "%s/error_%s.log.arc", log_ctx->log_file_path, now_time_string);
	rename( (const char *)log_ctx->log_file_name_error, (const char *)archive_name );

    open_log_file(log_ctx);

	return 0;

}

