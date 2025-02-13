ifndef SDE_INSTALL
$(error SDE_INSTALL is not set)
endif

P4_PROG  = netcache-controller
ARCH     = tofino2

CUR_DIR  = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SRC_DIR  = $(CUR_DIR)/src
OUT_DIR  = $(CUR_DIR)/build

HEADERS  = $(shell find $(SRC_DIR) -name "*.h")
SOURCES  = $(shell find $(SRC_DIR) -name "*.cpp")
OBJECTS  = $(foreach obj,$(patsubst %.cpp, %.o, $(SOURCES)),$(OUT_DIR)/$(shell basename -- $(obj)))
EXE      = $(OUT_DIR)/$(P4_PROG)

CC       = g++
CFLAGS   = -std=c++11                              	\
           -DSDE_INSTALL=\"$(SDE_INSTALL)\"         \
           -I$(SDE_INSTALL)/include                 \
           -I$(SDE_INSTALL)/include/$(ARCH)/pdfixed

# BF_LIBS  = -ldriver -lbfsys
BF_LIBS  = -ldriver

LDFLAGS  = -Wl,-rpath,$(SDE_INSTALL)/lib

LDLIBS   = -L$(SDE_INSTALL)/lib -L$(SDE_INSTALL)/lib/$(ARCH)pd/$(P4_PROG)
# LDLIBS   = -L$(SDE_INSTALL)/lib
LDLIBS  += -Wl,--start-group $(BF_LIBS) -Wl,--end-group
LDLIBS  += -lm -pthread -lpcap -ldl -levent

all: release

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(EXE)

release: CFLAGS += -O3
release: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -c $< $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(EXE) *.log bf_drivers* zlog-cfg-cur $(OUT_DIR)
