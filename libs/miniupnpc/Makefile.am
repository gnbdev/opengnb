if WINDOWS
AM_CFLAGS = -DMINIUPNP_STATICLIB
endif

noinst_LIBRARIES = libminiupnpc.a

libminiupnpc_a_SOURCES = \
			 connecthostport.c  \
			 igd_desc_parse.c \
			 minisoap.c \
			 minissdpc.c \
			 miniupnpc.c \
			 miniwget.c \
			 minixml.c \
			 portlistingparse.c \
			 receivedata.c \
			 upnpcommands.c \
			 upnpdev.c \
			 upnperrors.c \
			 upnpreplyparse.c

noinst_HEADERS = \
		 codelength.h \
		 connecthostport.h \
		 igd_desc_parse.h \
		 minisoap.h \
		 minissdpc.h \
		 miniupnpc_declspec.h \
		 miniupnpc.h \
		 miniupnpc_socketdef.h \
		 miniupnpcstrings.h \
		 miniupnpcstrings.h.in \
		 miniupnpctypes.h \
		 miniwget.h \
		 miniwget_private.h \
		 minixml.h \
		 portlistingparse.h \
		 receivedata.h \
		 upnpcommands.h \
		 upnpdev.h \
		 upnperrors.h \
		 upnpreplyparse.h

EXTRA_DIST = \
	     LICENSE \
	     VERSION
