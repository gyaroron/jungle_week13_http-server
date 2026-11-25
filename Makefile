CC      := cc
CSTD    := -std=c11
WARN    := -Wall -Wextra -Wpedantic
THREAD_FLAGS := -pthread
CFLAGS  := $(CSTD) $(WARN) -g -O0 -Iinclude $(THREAD_FLAGS)
LDFLAGS := $(THREAD_FLAGS)

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
BIN := build/server
TEST_HTTP_BIN := build/test_http
TEST_MIME_BIN := build/test_mime
TEST_ROUTE_BIN := build/test_route
TEST_HTTP_SRC := tests/test_http.c src/http.c
TEST_MIME_SRC := tests/test_mime.c src/mime.c
TEST_ROUTE_SRC := tests/test_route.c src/route.c

.PHONY: all asan tsan run clean test test-asan

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

asan: CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address,undefined
asan: clean all

tsan: CFLAGS += -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS += -fsanitize=thread
tsan: clean all

$(TEST_HTTP_BIN): $(TEST_HTTP_SRC) include/http.h | build
	$(CC) $(CFLAGS) $(TEST_HTTP_SRC) $(LDFLAGS) -o $@

$(TEST_MIME_BIN): $(TEST_MIME_SRC) include/mime.h | build
	$(CC) $(CFLAGS) $(TEST_MIME_SRC) $(LDFLAGS) -o $@

$(TEST_ROUTE_BIN): $(TEST_ROUTE_SRC) include/route.h | build
	$(CC) $(CFLAGS) $(TEST_ROUTE_SRC) $(LDFLAGS) -o $@

test: $(TEST_HTTP_BIN) $(TEST_MIME_BIN) $(TEST_ROUTE_BIN)
	./$(TEST_HTTP_BIN)
	./$(TEST_MIME_BIN)
	./$(TEST_ROUTE_BIN)

test-asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
test-asan: LDFLAGS += -fsanitize=address,undefined
test-asan: clean test

run: all
	./$(BIN)

clean:
	rm -rf build
