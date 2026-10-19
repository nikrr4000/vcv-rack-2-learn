# Atlas — the course inside VCV Rack. Built against the Rack SDK from vcv/.sdk.
RACK_DIR ?= ../../vcv/.sdk/Rack-SDK

FLAGS += -Isrc -Idep
SOURCES += $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
