# Rack-free tests and the render harness (plan §10.3, §10.4 fallback). Run from anywhere:
#   make -C plugin/tests unit      build + run unit tests
#   make -C plugin/tests render    build the preview renderer (bin/render)
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
CORE_SRC := $(filter-out $(SRC)/check/RackPatch.cpp,$(wildcard $(SRC)/text/*.cpp) $(wildcard $(SRC)/check/*.cpp) $(wildcard $(SRC)/core/*.cpp) $(wildcard $(SRC)/figures/*.cpp) $(wildcard $(SRC)/views/*.cpp) $(wildcard $(SRC)/ui/Draw.cpp))
JANSSON_SRC := $(wildcard $(V)/jansson/*.c)
VENDOR_OBJ := $(OBJ)/nanovg.o $(patsubst $(V)/jansson/%.c,$(OBJ)/jansson_%.o,$(JANSSON_SRC))
HARNESS_SRC := $(ROOT)/tests/render/SoftNvg.cpp

.PHONY: unit render clean
unit: $(BIN)/unit
	cd $(ROOT)/.. && $(BIN)/unit $(ONLY)

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

# Plugin smoke test: loads plugin.dylib with the Rack app's libRack (no window), creates every
# module and widget (with a module and as a preview), round-trips module data.
RACK_APP_RES ?= $(HOME)/Applications/VCV Rack 2 Free.app/Contents/Resources
RACK_SDK := $(ROOT)/../.sdk/Rack-SDK
.PHONY: smoke
smoke: $(BIN)/smoke
	@mkdir -p $(BIN)/rackrun && ln -sf "$(RACK_APP_RES)/libRack.dylib" $(BIN)/rackrun/libRack.dylib
	cd $(BIN)/rackrun && ATLAS_PACK=$(or $(PACK),$(ROOT)/tests/fixtures/pack-sample.json) $(BIN)/smoke $(ROOT)/plugin.dylib

$(BIN)/smoke: $(ROOT)/tests/smoke/smoke.cpp
	@mkdir -p $(BIN)
	@$(CXX) -std=c++11 -I$(RACK_SDK)/include -I$(RACK_SDK)/dep/include $< -L$(RACK_SDK) -lRack -o $@

# The plugin's Atlas client against a running Atlas server (tests/client/run.mjs starts one).
.PHONY: client
client: $(BIN)/client_e2e
	@mkdir -p $(BIN)/rackrun && ln -sf "$(RACK_APP_RES)/libRack.dylib" $(BIN)/rackrun/libRack.dylib
	node $(ROOT)/tests/client/run.mjs $(BIN)/client_e2e

$(BIN)/client_e2e: $(ROOT)/tests/client/client_e2e.cpp $(SRC)/net/Client.cpp $(SRC)/net/Client.hpp
	@mkdir -p $(BIN)
	@$(CXX) -std=c++11 -I$(SRC) -I$(RACK_SDK)/include -I$(RACK_SDK)/dep/include $(ROOT)/tests/client/client_e2e.cpp $(SRC)/net/Client.cpp -L$(RACK_SDK) -lRack -o $@
