#ifndef gnb_worker_type_h
#define gnb_worker_type_h

typedef struct _gnb_worker_t gnb_worker_t;

typedef struct _gnb_ring_buffer_t gnb_ring_buffer_t;

typedef void(*gnb_worker_init_func_t)(gnb_worker_t *gnb_worker, void *ctx);

typedef void(*gnb_worker_release_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_start_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_stop_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_notify_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_notify_func_t)(gnb_worker_t *gnb_worker);

typedef struct _gnb_worker_t {

	const char *name;

	gnb_worker_init_func_t      init;

	gnb_worker_release_func_t   release;

	gnb_worker_start_func_t     start;

	gnb_worker_stop_func_t      stop;

	gnb_worker_notify_func_t    notify;

	gnb_ring_buffer_t *ring_buffer;

	volatile int thread_worker_flag;

	volatile int thread_worker_ready_flag;

	volatile int thread_worker_run_flag;

	void *ctx;

}gnb_worker_t;


#endif


