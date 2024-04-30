BIN := lvl
CC := gcc
CFLAGS := `pkg-config --cflags gtk+-3.0 python3-embed`
LIBS := `pkg-config --libs gtk+-3.0 python3-embed`

DESTDIR :=
PREFIX := /usr/local

all: embedded-python.h $(BIN)

$(BIN): lvl.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

embedded-python.h: embedded_python.py
	xxd -i $< > $@

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	$(RM) $(BIN) embedded-python.h

.PHONY: all install uninstall clean
