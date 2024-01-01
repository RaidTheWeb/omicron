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
		 -Ilibs/KTX-Software/include \
		 -Ilibs/glfw/include \
		 -Ilibs/assimp/include \
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

OMICRON_RENDERBACKEND ?= vulkan
ifeq ($(strip $(OMICRON_RENDERBACKEND)), vulkan)
CFLAGS += -DOMICRON_RENDERVULKAN=1
else ifeq ($(strip $(OMICRON_RENDERBACKEND)), opengl)
CFLAGS += -DOMICRON_RENDEROPENGL=1
endif

# CFLAGS += -DOMICRON_RENDERBACKENDBGFX=1

CFLAGS += -DOMICRON_RENDERBACKENDVULKAN=1

.PHONY: clean utils

all: $(BIN_DIR)/$(OUT)

run: shaders $(BIN_DIR)/$(OUT)
	@$(BIN_DIR)/$(OUT)

utils:
	@$(CC) -o utils/rpak utils/rpak.c -lz

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
