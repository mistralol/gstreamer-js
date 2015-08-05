
plugin_LTLIBRARIES=libgstreamerjs.la

libgstreamerjs_la_SOURCES=bufferjitter.c \
		bufferspike.c \
		clockdrift.c \
		drop2key.c \
		dropdeltas.c \
		internalcommon.c \
		internalsink.c \
		internalsrc.c \
		pipestats.c \
		register.c

libgstreamerjs_la_CFLAGS = $(GST_CFLAGS)
libgstreamerjs_la_LIBADD = $(GST_LIBS)
libgstreamerjs_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstreamerjs_la_LIBTOOLSFLAGS=--tag=disable-static


