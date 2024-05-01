BIN := lvl
CC := gcc
CFLAGS := `pkg-config --cflags gtk+-3.0 sqlite3 libcjson libcurl`
LIBS := `pkg-config --libs gtk+-3.0 sqlite3 libcjson libcurl`

DESTDIR :=
PREFIX := /usr/local

SRC_DIR := ./src
OBJ_DIR := ./obj

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	$(RM) -r $(BIN) $(OBJ_DIR)

.PHONY: all install uninstall clean
