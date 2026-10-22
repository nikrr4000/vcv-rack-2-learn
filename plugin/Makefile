# Atlas — the course inside VCV Rack. Built against the Rack SDK from .sdk (scripts/get-sdk.sh).
RACK_DIR ?= ../.sdk/Rack-SDK

FLAGS += -Isrc -Idep
SOURCES += $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
