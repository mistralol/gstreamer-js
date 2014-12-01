

CC=gcc
OBJS=	pipestats.o \
	internalsink.o \
	dropdeltas.o \
	drop2key.o \
	clockdrift.o \
	bufferjitter.o \
	register.o


CFLAGS+=-fPIC -DPIC -Wall -ggdb -pipe
CFLAGS+=$(shell pkg-config --cflags gstreamer-1.0)

LIBS+=$(shell pkg-config --libs gstreamer-1.0)

all: gstreamer-custom.so

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@ -Wno-deprecated-declarations 


gstreamer-custom.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LIBS)


clean:
	$(RM) gstreamer-custom.so *.o



