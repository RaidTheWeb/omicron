
#
# Generic Make
#

SOURCE_DIR = src
BIN_DIR = bin
BUILD_DIR = build
CC = gcc
CXX = g++
CFLAGS = -std=gnu++17 -I$(SOURCE_DIR)/include -I$(SOURCE_DIR)/
DEBUG_CFLAGS =
PROD_CFLAGS = -O2
LDFLAGS =

#
# Project Specific
#

# Binary output filename
OUT = omicron
DEBUG ?= 1
# Project flags
CFLAGS +=
DEBUG_CFLAGS +=
PROD_CFLAGS +=
LDFLAGS +=

#
# Engine Specific
#

# Engine version
VERSION = 0.0.1
# Engine flags
CFLAGS += -DOMICRON_VERSION=\"$(VERSION)\" \
		  -Ilibs/glm \
		  -Ilibs/KTX-Software/include \
		  -Ilibs/glfw/include \
		  -Ilibs/assimp/include \
		  -Ilibs/tracy/public \
		  -Ilibs/imgui \
		  -Lrt/ \
		  -DKHRONOS_STATIC
DEBUG_CFLAGS += -g -DOMICRON_DEBUG=1 -DTRACY_ENABLE=1 -DTRACY_FIBERS
PROD_CFLAGS +=
LDFLAGS += -lm -lz \
		   -lglfw3 \
		   -lktx \
		   -lktx_read \
		   -lassimp \
		   -lX11 \
		   -lGL

#
# Actual work
#

HEADERS = $(shell find $(SOURCE_DIR)/include -type f -name '*.hpp')
SOURCES = $(shell find $(SOURCE_DIR) -type f -name '*.cpp') libs/imgui/imgui.cpp libs/imgui/imgui_draw.cpp libs/imgui/imgui_tables.cpp libs/imgui/imgui_widgets.cpp libs/tracy/public/TracyClient.cpp
OBJECTS = $(SOURCES:.cpp=.o)

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += $(DEBUG_CFLAGS)
else
	CFLAGS += $(PROD_CFLAGS)
endif

.PHONY: clean utils

all: utils libs $(BIN_DIR)/$(OUT)

libs/libsbuilt: # making this rule here so that we will attempt to create this file with the python script.
	@cd libs/ && python3 buildlibs.py

libs: libs/libsbuilt

run: libs $(BIN_DIR)/$(OUT)
	@./$(BIN_DIR)/$(OUT)

$(BIN_DIR)/$(OUT): $(OBJECTS)
	@printf "%8s %-40s %s %s\n" $(CXX) $@ "$(CFLAGS)" "$(LDFLAGS)"
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@chmod +x $(BIN_DIR)/$(OUT)

%.o: %.c $(HEADERS)
	@printf "%8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@mkdir -p $(BUILD_DIR)/
	@$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp $(HEADERS)
	@printf "%8s %-40s %s\n" $(CXX) $< "$(CFLAGS)"
	@mkdir -p $(BUILD_DIR)/
	@$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf bin
	rm -rf build
	rm -rf $(OBJECTS)

