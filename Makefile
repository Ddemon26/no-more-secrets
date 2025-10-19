# Installation directories following GNU conventions
prefix ?= /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
includedir = $(prefix)/include
mandir = $(datarootdir)/man

BIN=bin
OBJ=obj
SRC=src

CC ?= gcc
CFLAGS ?= -Wextra -Wall -O2

ifeq ($(OS),Windows_NT)
	EXEEXT=.exe
	TERMIO_SRC=nmstermio_win
	PLATFORM_LIBS=-lkernel32 -luser32
else
	EXEEXT=
	TERMIO_SRC=nmstermio
	PLATFORM_LIBS=
endif

NMS_EXE=$(BIN)/nms$(EXEEXT)
SNEAKERS_EXE=$(BIN)/sneakers$(EXEEXT)

.PHONY: all install uninstall clean nms sneakers all-ncurses nms-ncurses sneakers-ncurses

all: nms sneakers

nms: $(NMS_EXE)

sneakers: $(SNEAKERS_EXE)

all-ncurses: nms-ncurses sneakers-ncurses

$(NMS_EXE): $(OBJ)/input.o $(OBJ)/error.o $(OBJ)/nmscharset.o $(OBJ)/$(TERMIO_SRC).o $(OBJ)/nmseffect.o $(OBJ)/nms.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(PLATFORM_LIBS)

$(SNEAKERS_EXE): $(OBJ)/nmscharset.o $(OBJ)/$(TERMIO_SRC).o $(OBJ)/nmseffect.o $(OBJ)/sneakers.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(PLATFORM_LIBS)

nms-ncurses: $(OBJ)/input.o $(OBJ)/error.o $(OBJ)/nmscharset.o $(OBJ)/nmstermio_ncurses.o $(OBJ)/nmseffect.o $(OBJ)/nms.o | $(BIN)
	$(CC) $(CFLAGS) -o $(NMS_EXE) $^ -lncursesw

sneakers-ncurses: $(OBJ)/nmscharset.o $(OBJ)/nmstermio_ncurses.o $(OBJ)/nmseffect.o $(OBJ)/sneakers.o | $(BIN)
	$(CC) $(CFLAGS) -o $(SNEAKERS_EXE) $^ -lncursesw

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BIN):
	mkdir $(BIN)

$(OBJ):
	mkdir $(OBJ)

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)

install:
	install -d $(DESTDIR)$(mandir)/man6
	install -m644 nms.6 sneakers.6 $(DESTDIR)$(mandir)/man6 
	install -d $(DESTDIR)$(bindir)
	cd $(BIN) && install * $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/nms;
	rm -f $(DESTDIR)$(bindir)/sneakers;
	rm -f $(DESTDIR)$(mandir)/man6/nms.6
	rm -f $(DESTDIR)$(mandir)/man6/sneakers.6
