BIN := lvl

CC := gcc
CFLAGS := `pkg-config --cflags gtk+-3.0`
LIBS := `pkg-config --libs gtk+-3.0`

DESTDIR :=
PREFIX := /usr/local

all: $(BIN)

$(BIN): lvl.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	$(RM) $(BIN)

.PHONY: all clean
