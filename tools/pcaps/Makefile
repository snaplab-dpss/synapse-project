SOURCES = $(wildcard *.cpp)
BUILD_DIR = build
EXECS = $(addprefix $(BUILD_DIR)/,$(basename $(SOURCES)))

CC = g++

CFLAGS = -std=c++20
LIBS = -lpcap

release: CFLAGS += -O3
release: all

debug: CFLAGS += -g -O0 -DDEBUG
debug: all

all: $(EXECS)

build/%: %.cpp util.hpp
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
