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
CFLAGS = -g -std=gnu++17 -I$(SOURCE_DIR)/include -I$(SOURCE_DIR)/ \
		 -Ilibs/glm/ \
		 -Ilibs/bgfx/include \
		 -Ilibs/bx/include \
		 -Ilibs/KTX-Software/include \
		 -Ilibs/meshoptimizer/src \
		 -Ilibs/luajit/src \
		 -Lrt/ \
		 -DOMICRON_VERSION=$(VERSION)
		 # -Llibs/meshoptimizer/build \
		 # -Llibs/bgfx/.build/linux64_gcc/bin/

# bgfx needs to be compiled with the shared library copied to /usr/lib (or whatever else is the main path for libraries)
LDFLAGS = -lglfw \
		  -lktx \
		  -lktx_read \
		  -lassimp \
		  -lz \
		  -lX11 \
		  -lGL \
		  -lm \
		  -lmeshoptimizer \
		  -lluajit

HEADERS = $(shell find $(SOURCE_DIR)/include -type f -name '*.hpp')
SOURCES = $(shell find $(SOURCE_DIR) -type f -name '*.cpp')
# OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))
OBJECTS = $(SOURCES:.cpp=.o)

# override SPACE := $(subst ,, )
# MKESCAPE = $(subst $(SPACE),\ ,$(1))
# SHESCAPE = $(subst ','\'',$(1))
# OBJESCAPE = $(subst .a ,.a' ',$(subst .o ,.o' ',$(call SHESCAPE,$(1))))


# HEADERS = $(shell cd '$(call MKESCAPE,$(SOURCE_DIR))/include' && find -L * -type f -name '*.h')
# SOURCES = $(shell cd '$(call MKESCAPE,$(SOURCE_DIR))/' && find -L * -type f -name '*.c')
# OBJECTS = $(addprefix $(call MKESCAPE,$(BUILD_DIR))/,$(SOURCES:.c=.o))

SHADER_DIR = assets/shaders
SHADERS = $(shell find $(SHADER_DIR)/* -maxdepth 1 | grep -E ".*/(vs|fs|cs).*.sc")
SHADEROBJS = $(SHADERS:.sc=.bin)
SHADERC = libs/bgfx/.build/linux64_gcc/bin/shadercRelease
SHADERPLATFORM ?= linux
OMICRON_RENDERBACKEND ?= vulkan
ifeq ($(strip $(OMICRON_RENDERBACKEND)), vulkan)
SHADERPROFILE ?= spirv
CFLAGS += -DOMICRON_RENDERVULKAN=1
else ifeq ($(strip $(OMICRON_RENDERBACKEND)), opengl)
SHADERPROFILE ?= 430
CFLAGS += -DOMICRON_RENDEROPENGL=1
endif

# CFLAGS += -DOMICRON_RENDERBACKENDBGFX=1
LDFLAGS += -lbgfx

CFLAGS += -DOMICRON_RENDERBACKENDVULKAN=1

.PHONY: clean

all: shaders symbols $(BIN_DIR)/$(OUT)

shaders: $(SHADEROBJS)

%.bin: %.sc
	$(SHADERC) --type $(shell echo $(notdir $@) | cut -c 1) \
		-i libs/bgfx/src \
			--platform $(SHADERPLATFORM) \
			-p $(SHADERPROFILE) \
			--varyingdef $(dir $@)varying.def.sc \
			-f $< \
			-O3 \
			-o $@

symbols: $(HEADERS)
	@python3 parsesymbols.py $(HEADERS)

# run with mangohud
run-stat: $(BIN_DIR)/$(OUT)
	@mangohud --dlsym $(BIN_DIR)/$(OUT)

run: shaders $(BIN_DIR)/$(OUT)
	@$(BIN_DIR)/$(OUT)

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
	rm -r $(shell find $(SHADER_DIR)/* -maxdepth 1 | grep -E ".*/(vs|fs|cs).*.bin")
