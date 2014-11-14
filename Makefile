

CC=gcc
OBJS=	pipestats.o \
	internalsink.o \
	dropdeltas.o \
	clockdrift.o \
	register.o


CFLAGS+=-fPIC -DPIC -Wall -ggdb -pipe

all: gstreamer-custom.so

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@ -Wno-deprecated-declarations `pkg-config --cflags gstreamer-1.0`


gstreamer-custom.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS) `pkg-config --libs gstreamer-1.0`


clean:
	$(RM) gstreamer-custom.so *.o



