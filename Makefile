#
#	Game Makefile
#

OUT = omicron
VERSION = 1
SOURCE_DIR = src
BIN_DIR = bin
BUILD_DIR = build
CC = gcc
CXX = g++
CFLAGS = -std=gnu++17 -I$(SOURCE_DIR)/include -I$(SOURCE_DIR)/ \
		 -Ilibs/glm/ \
		 -Ilibs/KTX-Software/include \
		 -Ilibs/glfw/include \
		 -Ilibs/assimp/include \
		 -Ilibs/tracy/public/ \
		 -Ilibs/imgui \
		 -Ilibs/im3d \
		 -Lrt/ \
		 -DOMICRON_VERSION=$(VERSION)
		 # -Llibs/meshoptimizer/build \

# bgfx needs to be compiled with the shared library copied to /usr/lib (or whatever else is the main path for libraries)
LDFLAGS = -lglfw \
		  -lktx \
		  -lktx_read \
		  -lassimp \
		  -lz \
		  -lX11 \
		  -lGL \
		  -lm \


DEBUG ?= 1

HEADERS = $(shell find $(SOURCE_DIR)/include -type f -name '*.hpp')
SOURCES = $(shell find $(SOURCE_DIR) -type f -name '*.cpp') libs/imgui/imgui.cpp libs/imgui/imgui_draw.cpp libs/imgui/imgui_tables.cpp libs/imgui/imgui_widgets.cpp libs/tracy/public/TracyClient.cpp

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -DTRACY_ENABLE=1 -DTRACY_FIBERS
endif
SHADERS = $(shell find $(SOURCE_DIR)/engine/shaders -type f -name '*.glsl')
OSHADERS = $(SHADERS:.glsl=.spv)
OBJECTS = $(SOURCES:.cpp=.o)

OMICRON_RENDERBACKEND ?= vulkan
ifeq ($(strip $(OMICRON_RENDERBACKEND)), vulkan)
	CFLAGS += -DOMICRON_RENDERVULKAN=1 -DOMICRON_RENDERBACKENDVULKAN=1
else ifeq ($(strip $(OMICRON_RENDERBACKEND)), opengl)
	CFLAGS += -DOMICRON_RENDEROPENGL=1
endif

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -g -DOMICRON_DEBUG=1
else
	CFLAGS += -O2
endif

.PHONY: clean utils

all: utils $(OSHADERS) $(BIN_DIR)/$(OUT)

run: $(OSHADERS) $(BIN_DIR)/$(OUT)
	@bash run.sh

utils: utils/rpak

utils/rpak: utils/rpak.c
	@echo "Compiling utils/rpak"
	@$(CC) -o utils/rpak utils/rpak.c -lz

%.spv: %.glsl
	@printf "Compiling GLSL shader %s to Vulkan SPIRV\n" $^
	@glslc -I src/engine/shaders -fshader-stage=$(shell python3 src/engine/shaders/getshaderstage.py $^) -o $@ $^

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
	rm -r bin
	# rm -r build
	rm -r $(OBJECTS)
