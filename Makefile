# Automagically pick clang or gcc, with preference for clang
# This is only done if we have not overridden these with an environment or CLI variable
ifeq ($(origin CC),default)
	CC=$(shell if [ -e /usr/bin/clang ]; then echo clang; else echo gcc; fi)
endif
ifeq ($(origin CXX),default)
	CXX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

OSTYPE=$(shell uname -s | tr '[A-Z]' '[a-z]')
BUILD=build/$(OSTYPE)
CFLAGS=-g
INCLUDES=-I. -Iext/libzt/include
LIBS=-Lext/libzt/$(BUILD) -lzt -lpthread -lncurses -lportaudio -lm -ldl

CONFIG_INSTALL_DIR=$(HOME)/cathode

ifeq ($(OSTYPE),darwin)
	BIN_INSTALL_DIR=/usr/local/bin
	BUILD_OUTPUT_DIR=build/darwin
	BIN_DIST=dist/macOS
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif
ifeq ($(OSTYPE),linux)
	BIN_INSTALL_DIR=$(HOME)/bin
	BUILD_OUTPUT_DIR=build/linux
	BIN_DIST=dist/linux/
	ZTSDK_NETWORK_DIR=$(CONFIG_INSTALL_DIR)/networks.d
endif

# If root
ifeq ($(shell id -g),0)
	BIN_INSTALL_DIR=/usr/sbin
endif

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
CFLAGS+=-I$(INCDIR) 
platform=$(shell uname -s)
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
CFLAGS+=-O2 -Wall -g

ifeq ($(platform), linux)
	CFLAGS+=-DPA_USE_ALSA
else
	CFLAGS+=-DPA_USE_COREAUDIO
endif

CFLAGS+=`pkg-config --cflags opencv`
CFLAGS_DEBUG+=-O0 -g3 -Werror -DDEBUG
LDFLAGS+=`pkg-config --libs opencv`

libzt:
	cd ext/libzt && make -j4 static_lib LIBZT_TRACE=1 LIBZT_DEBUG=1

all: cathode

.PHONY: all clean debug

debug: CC := $(CC) $(CFLAGS_DEBUG)
debug: clean cathode

$(OBJS): | $(OBJDIR)
$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(wildcard $(INCDIR)/*.h) Makefile
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -o $@

cathode: $(OBJS)
	mkdir -p $(BUILD_OUTPUT_DIR)
	$(CC) $(CFLAGS) $^ -o $(BUILD_OUTPUT_DIR)/$@ $(LDFLAGS) $(LIBS)

video: CFLAGS := $(CFLAGS) -DVIDEOONLY
video: $(filter-out objs/cathode.o, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

audio: CFLAGS := $(CFLAGS) -DAUDIOONLY
audio: $(filter-out objs/cathode.o, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

install:
	@mkdir -p $(ZTSDK_NETWORK_DIR)
	@mkdir -p $(BIN_INSTALL_DIR)
	@cp -f $(INSTALL_BIN_SOURCE)/cathode $(BIN_INSTALL_DIR)/cathode
	@echo " - configuration files will be written to: " $(CONFIG_INSTALL_DIR)
	@echo " - cathode installed as: " $(BIN_INSTALL_DIR)/cathode

uninstall:
	rm -rf $(CONFIG_INSTALL_DIR) $(BIN_INSTALL_DIR)/cathode

clean:
	rm -rf $(OBJDIR) audio video $(BUILD_OUTPUT_DIR)/cathode
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
	'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete	

