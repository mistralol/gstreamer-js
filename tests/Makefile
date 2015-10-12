

INCLUDE+=-I.
INCLUDE+=$(shell pkg-config --cflags gstreamer-1.0)

CPP=g++
CPP_OPTS+=-Wall -ggdb -O2
MKDEP=gcc -E -MM

LIBS+=-pthread
LIBS+=$(shell pkg-config --libs gstreamer-1.0)
LIBS+=$(shell pkg-config --libs gstreamer-app-1.0)

SRCS=	VideoPipeLine.cpp \
	test-internal-h264.cpp \
	test-internal-mjpeg.cpp
	

OBJS=$(SRCS:%.cpp=%.o)

all: test-internal-h264.exe test-internal-mjpeg.exe

%.o: %.cpp Makefile
	$(CPP) $(CPP_OPTS) $(INCLUDE) -o $@ -c $<

test-internal-mjpeg.exe: $(OBJS) test-internal-mjpeg.cpp .depend
	$(CPP) test-internal-mjpeg.o VideoPipeLine.o $(LIBS) -o $@

test-internal-h264.exe: $(OBJS) test-internal-h264.cpp .depend
	$(CPP) test-internal-h264.o VideoPipeLine.o $(LIBS) -o $@

dep: .depend

.depend: $(SRCS)
	$(MKDEP) $(INCLUDE) $(SRCS) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

clean:
	$(RM) *.o test-internal.exe .depend


