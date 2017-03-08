CC=g++
OBJDIR=objs
SRCDIR=p2pvc/src
INCDIR=$(SRCDIR)/inc

ZT_INCLUDE=ztsdk/
CFLAGS+=-I$(INCDIR) -I$(ZT_INCLUDE)

platform=$(shell uname -s)

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

CFLAGS+=-Wall -L.
ifeq ($(platform), Linux)
	CFLAGS+=-DPA_USE_ALSA
else
	CFLAGS+=-DPA_USE_COREAUDIO
endif

ZTSDK_LIB=-Lztsdk -lzt
CFLAGS+=`pkg-config --cflags opencv`
CFLAGS_DEBUG+=-O0 -g -Werror -DDEBUG
LDFLAGS+=-lpthread -lncurses -lportaudio -lm $(ZTSDK_LIB) -ldl
LDFLAGS+=`pkg-config --libs opencv`

all: cathode

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

clean:
	rm -rf $(OBJDIR) audio video cathode

