SUBDIRS = crypto

GNB_CORE = \
	   ../libs/hash/murmurhash.c \
	   gnb_address.c \
	   gnb_alloc.c \
	   gnb_arg_list.c \
	   gnb_conf.c \
	   gnb_conf_file.c \
	   gnb_config_lite.c \
	   gnb_ctl_block.c \
	   gnb_daemon.c \
	   gnb_detect_worker.c \
	   gnb_dir.c \
	   gnb_doubly_linked_list.c \
	   gnb_fixed_pool.c \
	   gnb_hash32.c \
	   gnb_index_service_worker.c \
	   gnb_index_worker.c \
	   gnb_keys.c \
	   gnb_log.c \
	   gnb_lru32.c \
	   gnb_main_worker.c \
	   gnb_mmap.c \
	   gnb_node.c \
	   gnb_node_worker.c \
	   gnb_payload16.c \
	   gnb_ring_buffer.c \
	   gnb_svr.c \
	   gnb_time.c \
	   gnb_udp.c \
	   gnb_worker.c

GNB_CORE += gnb_mod_register.c

GNB_PF = \
	 gnb_pf.c \
	 gnb_unified_forwarding.c \
	 packet_filter/gnb_pf_crypto_arc4.c \
	 packet_filter/gnb_pf_crypto_xor.c \
	 packet_filter/gnb_pf_dump.c \
	 packet_filter/gnb_pf_route.c

bin_PROGRAMS = gnb gnb_es gnb_crypto gnb_ctl

gnb_SOURCES = \
	      $(GNB_CORE) \
	      cli/gnb.c \
	      gnb_argv.c

EXTRA_gnb_SOURCES = gnb.exe.manifest gnb_res.rc

if LINUX
gnb_SOURCES += unix/unix_platform.c linux/gnb_drv_linux.c
endif
if WINDOWS
gnb_SOURCES += mingw/windows_platform.c mingw/gnb_drv_win32.c mingw/gnb_drv_wintun.c
endif
if DARWIN
gnb_SOURCES += unix/unix_platform.c Darwin/gnb_drv_darwin.c
endif
if FREEBSD
gnb_SOURCES += unix/unix_platform.c freebsd/gnb_drv_freebsd.c
endif
if OPENBSD
gnb_SOURCES += unix/unix_platform.c openbsd/gnb_drv_openbsd.c
endif
gnb_SOURCES += $(GNB_PF) gnb_binary.c

gnb_CFLAGS = -I$(top_srcdir)/libs
gnb_LDFLAGS = -pthread

gnb_LDADD = $(top_srcdir)/libs/ed25519/libed25519.a
gnb_LDADD += crypto/libgnb_crypto.a
if WINDOWS
gnb_LDADD += gnb_res.o -lws2_32 -liphlpapi
endif

gnb_crypto_SOURCES = gnb_crypto.c gnb_binary.c
gnb_crypto_CFLAGS = -I$(top_srcdir)/libs
gnb_crypto_LDADD = $(top_srcdir)/libs/ed25519/libed25519.a
gnb_crypto_LDADD += crypto/libgnb_crypto.a

gnb_es_SOURCES = \
		 cli/gnb_es.c \
		 es/gnb_environment_service.c \
		 gnb_daemon.c \
		 es/gnb_es_dump.c \
		 es/gnb_es_broadcast_address.c \
		 es/gnb_es_resolv.c \
		 es/gnb_es_upnp.c \
		 es/gnb_discover_in_lan.c \
		 gnb_ctl_block.c \
		 gnb_time.c \
		 gnb_address.c \
		 gnb_payload16.c \
		 gnb_udp.c \
		 gnb_log.c \
		 gnb_alloc.c \
		 gnb_mmap.c \
		 gnb_dir.c \
		 gnb_hash32.c \
		 gnb_conf.c \
		 gnb_arg_list.c \
		 ../libs/hash/murmurhash.c \
		 es/gnb_es_platform.c \
		 es/gnb_es_type.h
gnb_es_SOURCES += gnb_binary.c
if LINUX
gnb_es_SOURCES += unix/unix_platform.c
endif
if WINDOWS
gnb_es_SOURCES += mingw/windows_platform.c
endif
if DARWIN
gnb_es_SOURCES += unix/unix_platform.c
endif
if FREEBSD
gnb_es_SOURCES += unix/unix_platform.c
endif
if OPENBSD
gnb_es_SOURCES += unix/unix_platform.c
endif

gnb_es_CFLAGS = -I$(top_srcdir)/libs/miniupnpc -I$(top_srcdir)/libs/libnatpmp -I$(top_srcdir)/libs
gnb_es_LDFLAGS = -pthread

gnb_es_LDADD = $(top_srcdir)/libs/ed25519/libed25519.a
gnb_es_LDADD += crypto/libgnb_crypto.a
gnb_es_LDADD += $(top_srcdir)/libs/miniupnpc/libminiupnpc.a
gnb_es_LDADD += $(top_srcdir)/libs/libnatpmp/libnatpmp.a
if WINDOWS
gnb_es_CFLAGS += -DMINIUPNP_STATICLIB
gnb_es_CFLAGS += -DNATPMP_STATICLIB
gnb_es_LDADD += gnb_res.o -lws2_32 -liphlpapi
endif

gnb_ctl_SOURCES = \
		  cli/gnb_ctl.c \
		  ctl/gnb_ctl_dump.c \
		  gnb_ctl_block.c \
		  gnb_ctl_block_set.c \
		  gnb_binary.c \
		  gnb_time.c \
		  gnb_address.c \
		  gnb_payload16.c \
		  gnb_udp.c \
		  gnb_mmap.c                   
gnb_ctl_CFLAGS = -I$(top_srcdir)/libs
gnb_ctl_LDADD = $(top_srcdir)/libs/ed25519/libed25519.a
if WINDOWS
gnb_ctl_LDADD += gnb_res.o -lws2_32
endif

noinst_HEADERS = \
		 gnb_address.h gnb_address_type.h gnb_alloc.h gnb_arg_list.h gnb_binary.h \
		 gnb_block.h gnb_conf_file.h gnb_config_lite.h gnb_conf_type.h \
		 gnb_core_frame_type_defs.h gnb_ctl_block.h gnb_dir.h \
		 gnb_discover_in_lan_frame_type.h gnb_doubly_linked_list.h gnb_exec.h \
		 gnb_fixed_pool.h gnb.h gnb_hash32.h gnb_index_frame_type.h gnb_keys.h \
		 gnb_log.h gnb_log_type.h gnb_lru32.h gnb_mmap.h gnb_node.h gnb_node_type.h \
		 gnb_payload16.h gnb_pf.h gnb_pingpong_frame_type.h gnb_platform.h \
		 gnb_ring_buffer.h gnb_svr.h gnb_time.h gnb_tun_drv.h gnb_udp.h \
		 gnb_uf_node_frame_type.h gnb_unified_forwarding.h gnb_ur1_frame_type.h \
		 gnb_worker.h gnb_worker_queue_data.h gnb_worker_type.h

gnb_res.o: gnb_res.rc
	${WINDRES} gnb_res.rc -o gnb_res.o
