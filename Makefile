BIN := lvl
CC := gcc
CFLAGS := `pkg-config --cflags gtk+-3.0 python3-embed sqlite3`
LIBS := `pkg-config --libs gtk+-3.0 python3-embed sqlite3`

DESTDIR :=
PREFIX := /usr/local

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

all: embedded-python.h $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

embedded-python.h: embedded_python.py
	xxd -i $< > $@

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) embedded-python.h

.PHONY: all install uninstall clean
