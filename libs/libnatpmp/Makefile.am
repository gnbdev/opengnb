if WINDOWS
AM_CFLAGS = -DNATPMP_STATICLIB
endif

noinst_LIBRARIES = libnatpmp.a

libnatpmp_a_SOURCES = \
		      natpmp.c \
		      getgateway.c \
		      wingettimeofday.c

noinst_HEADERS = \
		 declspec.h \
		 getgateway.h \
		 natpmp.h \
		 wingettimeofday.h

EXTRA_DIST = LICENSE
