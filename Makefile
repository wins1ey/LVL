BIN := lvl
CC := gcc
CFLAGS := `pkg-config --cflags gtk+-3.0 python3-embed sqlite3`
LIBS := `pkg-config --libs gtk+-3.0 python3-embed sqlite3`

DESTDIR :=
PREFIX := /usr/local

SRC_DIR := ./src
OBJ_DIR := ./obj

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: steamapi.h $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

steamapi.h: $(SRC_DIR)/steamapi.py
	xxd -i $< > $(SRC_DIR)/$@

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	$(RM) -r $(BIN) $(OBJ_DIR) $(SRC_DIR)/steamapi.h

.PHONY: all install uninstall clean
