#ifndef gnb_log_h
#define gnb_log_h

#include <inttypes.h>

#include "gnb_log_type.h"

gnb_log_ctx_t* gnb_log_ctx_create();


#define GNB_LOG_TYPE_STD   0
#define GNB_LOG_TYPE_DEBUG 1
#define GNB_LOG_TYPE_ERROR 2


//level 越大，日志信息越详细,0不输出日志，暂时未用
#define GNB_LOG_LEVEL0          0
#define GNB_LOG_LEVEL1          1
#define GNB_LOG_LEVEL2          2
#define GNB_LOG_LEVEL3          3



/*
level 控制 console file udp 的输出

STD DEBUG ERROR 只是作为一种日志的内置标签，不通过 level 细分控制

要控制 debug 输出，可以让STD 使用小的level，DEBUG用高的level


STD    level = 1
ERROR  level = 2
DEBUG  level = 3
*/


void gnb_logf(gnb_log_ctx_t *log, uint8_t log_type, uint8_t log_id, uint8_t level, const char *format, ...);

int gnb_log_udp_open(gnb_log_ctx_t *log);

int gnb_log_file_rotate(gnb_log_ctx_t *log);


int gnb_log_udp_set_addr4(gnb_log_ctx_t *log, char *ip, uint16_t port4);
int gnb_log_udp_set_addr6(gnb_log_ctx_t *log, char *ip, uint16_t port6);


int gnb_log_udp_set_addr4_string(gnb_log_ctx_t *log, char *sockaddress4_string);


#define GNB_STD1(log,log_id,format,...)                                                          \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL1 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL1 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL1)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_STD, log_id, GNB_LOG_LEVEL1, format, ##__VA_ARGS__);   \
			}                                                                                    \
        }while(0);


#define GNB_STD2(log,log_id,format,...)                                                          \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL2 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL2 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL2)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_STD, log_id, GNB_LOG_LEVEL2, format, ##__VA_ARGS__);   \
			}                                                                                    \
        }while(0);

#define GNB_STD3(log,log_id,format,...)                                                          \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL3 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL3 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL3)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_STD, log_id, GNB_LOG_LEVEL3, format, ##__VA_ARGS__);   \
			}                                                                                    \
        }while(0);




#define GNB_DEBUG1(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL1 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL1 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL1)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_DEBUG, log_id, GNB_LOG_LEVEL1, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);


#define GNB_DEBUG2(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL2 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL2 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL2)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_DEBUG, log_id, GNB_LOG_LEVEL2, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);

#define GNB_DEBUG3(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL3 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL3 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL3)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_DEBUG, log_id, GNB_LOG_LEVEL3, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);




#define GNB_ERROR1(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL1 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL1 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL1)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_ERROR, log_id, GNB_LOG_LEVEL1, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);


#define GNB_ERROR2(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL2 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL2 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL2)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_ERROR, log_id, GNB_LOG_LEVEL2, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);

#define GNB_ERROR3(log,log_id,format,...)                                                        \
        do{                                                                                      \
			if ( NULL != log && GNB_LOG_OUTPUT_NONE != log->output_type &&                       \
				 (log->config_table[log_id].console_level >= GNB_LOG_LEVEL3 ||                   \
                  log->config_table[log_id].file_level    >= GNB_LOG_LEVEL3 ||                   \
				  log->config_table[log_id].udp_level     >= GNB_LOG_LEVEL3)                     \
                ){                                                                               \
				gnb_logf(log,GNB_LOG_TYPE_ERROR, log_id, GNB_LOG_LEVEL3, format, ##__VA_ARGS__); \
			}                                                                                    \
        }while(0);



#endif

