#ifndef gnb_log_type_h
#define gnb_log_type_h

#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#ifdef _WIN32

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#endif


typedef struct _gnb_log_ctx_t {

	#define GNB_LOG_OUTPUT_NONE      (0x0)
	#define GNB_LOG_OUTPUT_STDOUT    (0x1)
	#define GNB_LOG_OUTPUT_FILE      (0x1 << 1)
	#define GNB_LOG_OUTPUT_UDP       (0x1 << 2)
	unsigned char log_output;

	char log_file_path[PATH_MAX];

	//char log_file_prefix[32];

	char log_file_name_std[PATH_MAX+NAME_MAX];
	char log_file_name_debug[PATH_MAX+NAME_MAX];
	char log_file_name_error[PATH_MAX+NAME_MAX];

	//char log_file_name_raw[PATH_MAX+NAME_MAX];

	int fd_log_std;
	int fd_log_debug;
	int fd_log_error;

	//int fd_log_raw;

	int pre_mday;

}gnb_log_ctx_t;


#endif
