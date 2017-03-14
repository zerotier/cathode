OSTYPE=$(shell uname -s)

STACK_LIB=libpicotcp.so
ZTSDK_LIB=libzt.a
CATHODE_TMP=tmp/
ZT_INCLUDE=ztsdk/

ifeq ($(OSTYPE),Darwin)
	STACK_LIB_PATH+=ztsdk/lib/darwin.$(STACK_LIB)
	ZTSDK_LIB_PATH+=ztsdk/lib/darwin.$(ZTSDK_LIB)
endif
ifeq ($(OSTYPE),Linux)
	STACK_LIB_PATH=ztsdk/lib/linux.$STACK_LIB
	ZTSDK_LIB_PATH=ztsdk/lib/linux.$ZTSDK_LIB
endif

CC=clang++
OBJDIR=objs
SRCDIR=p2pvc/src
INCDIR=$(SRCDIR)/inc

CFLAGS+=-I$(INCDIR) -I$(ZT_INCLUDE)

platform=$(shell uname -s)

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

CFLAGS+=-O2 -Wall
ifeq ($(platform), Linux)
	CFLAGS+=-DPA_USE_ALSA
else
	CFLAGS+=-DPA_USE_COREAUDIO
endif

ZTSDKLIB=-Lztsdk -lzt

CFLAGS+=`pkg-config --cflags opencv`
CFLAGS_DEBUG+=-O0 -g3 -Werror -DDEBUG
LDFLAGS+=-lpthread -lncurses -lportaudio -lm $(ZTSDKLIB) -ldl
LDFLAGS+=`pkg-config --libs opencv`

all: configure cathode

.PHONY: all clean debug

debug: CC := $(CC) $(CFLAGS_DEBUG)
debug: clean cathode

cathode: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

video: CFLAGS := $(CFLAGS) -DVIDEOONLY
video: $(filter-out objs/cathode.o, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

audio: CFLAGS := $(CFLAGS) -DAUDIOONLY
audio: $(filter-out objs/cathode.o, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJS): | $(OBJDIR)
$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(wildcard $(INCDIR)/*.h) Makefile
	$(CC) $(CFLAGS) $< -c -o $@

install:

# Copy libraries into correct dirs for build and runtime
configure:
	cp $(STACK_LIB_PATH) $(CATHODE_TMP)$(STACK_LIB)
	cp $(ZTSDK_LIB_PATH) $(ZT_INCLUDE)$(ZTSDK_LIB)

clean:
	rm -rf $(OBJDIR) audio video cathode

