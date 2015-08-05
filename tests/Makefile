

INCLUDE+=-I.
INCLUDE+=$(shell pkg-config --cflags gstreamer-1.0)

CPP=g++
CPP_OPTS+=-Wall -ggdb -O2
MKDEP=gcc -E -MM

LIBS+=-pthread
LIBS+=$(shell pkg-config --libs gstreamer-1.0)
LIBS+=$(shell pkg-config --libs gstreamer-app-1.0)

SRCS=	test-internal.cpp \
	VideoPipeLine.cpp

OBJS=$(SRCS:%.cpp=%.o)

all: test-internal.exe

%.o: %.cpp Makefile
	$(CPP) $(CPP_OPTS) $(INCLUDE) -o $@ -c $<

test-internal.exe: $(OBJS) .depend
	$(CPP) $(OBJS) $(LIBS) -o $@

dep: .depend

.depend: $(SRCS)
	$(MKDEP) $(INCLUDE) $(SRCS) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

clean:
	$(RM) *.o test-internal.exe .depend


