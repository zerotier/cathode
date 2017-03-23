OSTYPE=$(shell uname -s)

ZT_INCLUDE_DIR=zt/
CONFIG_INSTALL_DIR=$(HOME)/cathode

STACK_LIB=libpicotcp.so
STACK_LIB_PATH=
ZTSDK_LIB=libzt.a
ZTSDK_LIB_PATH=

ifeq ($(OSTYPE),Darwin)
	BIN_INSTALL_DIR=/usr/local/bin
	BUILD_OUTPUT_DIR=build/macOS
	LIB_DIST=zt/macOS
	BIN_DIST=dist/macOS
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif
ifeq ($(OSTYPE),Linux)
	BIN_INSTALL_DIR=$(HOME)/bin
	BUILD_OUTPUT_DIR=build/linux
	LIB_DIST=zt/linux
	BIN_DIST=dist/linux/
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif


# If root
ifeq ($(shell id -g),0)
	BIN_INSTALL_DIR=/usr/sbin
endif


ZTSDKLIB=-L$(LIB_DIST) -lzt
STACK_LIB_PATH+=$(LIB_DIST)/$(STACK_LIB)
ZTSDK_LIB_PATH+=$(LIB_DIST)/$(ZTSDK_LIB)

# Check for presence of build/ binaries, if not, use dist/ for install
ifneq ("$(wildcard $(BUILD_OUTPUT_DIR)/cathode)","")
    INSTALL_BIN_SOURCE=$(BUILD_OUTPUT_DIR)
else
    INSTALL_BIN_SOURCE=$(BIN_DIST)
endif

CC=clang++
OBJDIR=objs
SRCDIR=p2pvc/src
INCDIR=$(SRCDIR)/inc
CFLAGS+=-I$(INCDIR) -I$(ZT_INCLUDE_DIR)
platform=$(shell uname -s)
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

CFLAGS+=-O2 -Wall -g

ifeq ($(platform), Linux)
	CFLAGS+=-DPA_USE_ALSA
else
	CFLAGS+=-DPA_USE_COREAUDIO
endif

CFLAGS+=`pkg-config --cflags opencv`
CFLAGS_DEBUG+=-O0 -g3 -Werror -DDEBUG
LDFLAGS+=-lpthread -lncurses -lportaudio -lm $(ZTSDKLIB) -ldl
LDFLAGS+=`pkg-config --libs opencv`

all: cathode

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
	@mkdir -p $(ZTSDK_NETWORK_DIR)
	@mkdir -p $(BIN_INSTALL_DIR)
	@cp -f $(STACK_LIB_PATH) $(CONFIG_INSTALL_DIR)/$(STACK_LIB)
	@cp -f $(INSTALL_BIN_SOURCE)/cathode $(BIN_INSTALL_DIR)/cathode
	@echo " - configuration files will be written to: " $(CONFIG_INSTALL_DIR)
	@echo " - cathode installed as: " $(BIN_INSTALL_DIR)/cathode

uninstall:
	rm -rf $(CONFIG_INSTALL_DIR) $(BIN_INSTALL_DIR)/cathode

clean:
	rm -rf $(OBJDIR) audio video $(BUILD_OUTPUT_DIR)/cathode

