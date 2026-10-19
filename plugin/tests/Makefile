# Rack-free tests and the render harness (plan §10.3, §10.4 fallback). Run from anywhere:
#   make -C rack/plugin/tests unit      build + run unit tests
#   make -C rack/plugin/tests render    build the preview renderer (bin/render)
ROOT := $(abspath ..)
SRC := $(ROOT)/src
V := $(ROOT)/tests/vendor
BIN := $(ROOT)/tests/bin
OBJ := $(BIN)/obj
CXX ?= clang++
CC ?= clang
CXXFLAGS := -std=c++17 -O2 -Wall -Wno-unused-function -I$(SRC) -I$(V)/nanovg -I$(V)/jansson -I$(ROOT)/tests/render -I$(ROOT)/tests/unit
CFLAGS := -O2 -I$(V)/nanovg -I$(V)/jansson -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_FCNTL_H=1 -DHAVE_SCHED_H=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_SCHED_YIELD=1 -DHAVE_ATOMIC_BUILTINS=1 -DHAVE_SYNC_BUILTINS=1 -DUSE_URANDOM=1 -DINITIAL_HASHTABLE_ORDER=3 -Wno-everything

# Rack-free plugin sources (everything except Rack glue).
CORE_SRC := $(wildcard $(SRC)/text/*.cpp) $(wildcard $(SRC)/check/*.cpp) $(wildcard $(SRC)/core/*.cpp) $(wildcard $(SRC)/figures/*.cpp) $(wildcard $(SRC)/views/*.cpp) $(wildcard $(SRC)/ui/Draw.cpp)
JANSSON_SRC := $(wildcard $(V)/jansson/*.c)
VENDOR_OBJ := $(OBJ)/nanovg.o $(patsubst $(V)/jansson/%.c,$(OBJ)/jansson_%.o,$(JANSSON_SRC))
HARNESS_SRC := $(ROOT)/tests/render/SoftNvg.cpp

.PHONY: unit render clean
unit: $(BIN)/unit
	cd $(ROOT)/../.. && $(BIN)/unit $(ONLY)

render: $(BIN)/render

$(OBJ)/nanovg.o: $(V)/nanovg/nanovg.c
	@mkdir -p $(OBJ)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/jansson_%.o: $(V)/jansson/%.c
	@mkdir -p $(OBJ)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/unit: $(wildcard $(ROOT)/tests/unit/*.cpp) $(wildcard $(ROOT)/tests/unit/*.hpp) $(CORE_SRC) $(HARNESS_SRC) $(VENDOR_OBJ) $(wildcard $(SRC)/*/*.hpp)
	@mkdir -p $(BIN)
	@$(CXX) $(CXXFLAGS) $(wildcard $(ROOT)/tests/unit/*.cpp) $(CORE_SRC) $(HARNESS_SRC) $(VENDOR_OBJ) -o $@

$(BIN)/render: $(wildcard $(ROOT)/tests/render/*.cpp) $(wildcard $(ROOT)/tests/render/*.hpp) $(CORE_SRC) $(VENDOR_OBJ) $(wildcard $(SRC)/*/*.hpp)
	@mkdir -p $(BIN)
	@$(CXX) $(CXXFLAGS) $(wildcard $(ROOT)/tests/render/*.cpp) $(CORE_SRC) $(VENDOR_OBJ) -o $@

clean:
	rm -rf $(BIN)
