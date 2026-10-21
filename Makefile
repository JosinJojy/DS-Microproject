# MazeLab Makefile
# Tested with: mingw32-make on Windows (MinGW GCC 6.3+)
#              GNU make on Linux/macOS

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
# Debug flags (no sanitizers available on MinGW 6.3)
CFLAGS_DEBUG = -std=c11 -Wall -Wextra -g -DDEBUG

# Windows-specific: link winsock2, define alloca for MinGW
# MinGW GCC does not expose alloca() by default; use compiler built-in.
WIN_DEFS  = -D_WIN32_WINNT=0x0601 "-Dalloca(n)=__builtin_alloca(n)"
WIN_LDFLAGS = -lws2_32

# Source directories
BACKEND_DIR    = backend
THIRD_PARTY    = third_party
MONGOOSE_DIR   = $(THIRD_PARTY)/mongoose
CJSON_DIR      = $(THIRD_PARTY)/cjson
TESTS_DIR      = tests

# Include paths
INCLUDES = -I$(BACKEND_DIR) -I$(MONGOOSE_DIR) -I$(CJSON_DIR)

# Backend sources (shared between server and test builds)
BACKEND_SRCS = \
    $(BACKEND_DIR)/maze.c    \
    $(BACKEND_DIR)/queue.c   \
    $(BACKEND_DIR)/stack.c   \
    $(BACKEND_DIR)/path.c    \
    $(BACKEND_DIR)/bfs.c     \
    $(BACKEND_DIR)/dfs.c     \
    $(BACKEND_DIR)/api.c     \
    $(MONGOOSE_DIR)/mongoose.c \
    $(CJSON_DIR)/cJSON.c

MAIN_SRC    = $(BACKEND_DIR)/main.c
TEST_SRC    = $(TESTS_DIR)/test_algorithms.c

TARGET      = mazelab.exe
TEST_TARGET = test_algorithms.exe

all: $(TARGET)

$(TARGET): $(BACKEND_SRCS) $(MAIN_SRC)
	$(CC) $(CFLAGS) $(WIN_DEFS) $(INCLUDES) $^ -o $@ $(WIN_LDFLAGS)
	@echo Build successful: $(TARGET)

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(BACKEND_SRCS) $(TEST_SRC)
	$(CC) $(CFLAGS_DEBUG) $(WIN_DEFS) $(INCLUDES) $^ -o $@ $(WIN_LDFLAGS)
	@echo Test build successful: $(TEST_TARGET)

clean:
	-del /f /q $(TARGET) $(TEST_TARGET) 2>NUL

.PHONY: all debug test clean
