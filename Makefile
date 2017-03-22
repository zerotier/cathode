OSTYPE=$(shell uname -s)

ZT_INCLUDE_DIR=zt/

CONFIG_INSTALL_DIR=/var/lib/ztsdk
BIN_INSTALL_DIR=/usr/local/bin

STACK_LIB=libpicotcp.so
STACK_LIB_PATH=
ZTSDK_LIB=libzt.a
ZTSDK_LIB_PATH=

ifeq ($(OSTYPE),Darwin)
	BUILD_OUTPUT_DIR=build/macOS
	LIB_DIST=zt/macOS
	BIN_DIST=dist/macOS
	CONFIG_INSTALL_DIR=/Users/Shared/cathode
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif
ifeq ($(OSTYPE),Linux)
	BUILD_OUTPUT_DIR=build/linux
	LIB_DIST=zt/linux
	BIN_DIST=dist/linux/
	CONFIG_INSTALL_DIR=/home/cathode
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif

ZTSDKLIB=-L$(LIB_DIST) -lzt
STACK_LIB_PATH+=$(LIB_DIST)/$(STACK_LIB)
ZTSDK_LIB_PATH+=$(LIB_DIST)/$(ZTSDK_LIB)

CC=clang++
OBJDIR=objs
SRCDIR=p2pvc/src
INCDIR=$(SRCDIR)/inc
CFLAGS+=-I$(INCDIR) -I$(ZT_INCLUDE_DIR)

platform=$(shell uname -s)

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

CFLAGS+=-O2 -Wall

ifeq ($(platform), Linux)
	CFLAGS+=-DPA_USE_ALSA
else
	CFLAGS+=-DPA_USE_COREAUDIO
endif

CFLAGS+=`pkg-config --cflags opencv`
CFLAGS_DEBUG+=-O0 -g3 -Werror -DDEBUG
LDFLAGS+=-lpthread -lncurses -lportaudio -lm $(ZTSDKLIB) -ldl
LDFLAGS+=`pkg-config --libs opencv`

all: cathode install

.PHONY: all clean debug

debug: CC := $(CC) $(CFLAGS_DEBUG)
debug: clean cathode

cathode: $(OBJS)
	mkdir -p $(BUILD_OUTPUT_DIR)
	$(CC) $(CFLAGS) $^ -o $(BUILD_OUTPUT_DIR)/$@ $(LDFLAGS)

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
	mkdir -p $(ZTSDK_NETWORK_DIR)
	cp -f $(STACK_LIB_PATH) $(CONFIG_INSTALL_DIR)/$(STACK_LIB)
	cp -f $(BUILD_OUTPUT_DIR)/cathode $(BIN_INSTALL_DIR)/cathode

uninstall:
	rm -rf $(CONFIG_INSTALL_DIR) $(BIN_INSTALL_DIR)/cathode

clean:
	rm -rf $(OBJDIR) audio video $(BUILD_OUTPUT_DIR)/cathode

