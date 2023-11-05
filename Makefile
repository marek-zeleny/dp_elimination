# Sources of inspiration:
# https://makefiletutorial.com/
# https://stackoverflow.com/a/2481326/11983817
# https://stackoverflow.com/a/28663974/11983817
# https://stackoverflow.com/a/10825154 (debug vs. release build)

ifndef BUILD_DIR

RELEASE_DIR := release/
DEBUG_DIR := debug/
ASAN_DIR := asan/
UBSAN_DIR := ubsan/

.PHONY: default all release debug asan ubsan clean

default all: release

release: export BUILD_DIR := $(RELEASE_DIR)
release: export EXTRA_FLAGS := -DNDEBUG -O3
debug:   export BUILD_DIR := $(DEBUG_DIR)
debug:   export EXTRA_FLAGS := -DDEBUG -g -O0
asan:    export BUILD_DIR := $(ASAN_DIR)
asan:    export EXTRA_FLAGS := -DDEBUG -g -O0 -fsanitize=address
ubsan:    export BUILD_DIR := $(UBSAN_DIR)
ubsan:    export EXTRA_FLAGS := -DDEBUG -g -O0 -fsanitize=undefined

release debug asan ubsan:
	@$(MAKE)

clean:
	rm -r $(RELEASE_DIR)
	rm -r $(DEBUG_DIR)
	rm -r $(ASAN_DIR)
	rm -r $(UBSAN_DIR)

else

APPNAME := dp
EXE := $(BUILD_DIR)$(APPNAME)

CC=gcc
CXX=g++

SRC_DIRS := src/

# Link external libraries
LIB_DIR := external/lib/
LIBS := $(LIB_DIR)sylvan/libsylvan.a \
		$(LIB_DIR)sylvan/liblace.a \
		$(LIB_DIR)sylvan/liblace14.a
INCLUDE := external/include/sylvan

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CFLAGS := $(INC_FLAGS) $(EXTRA_FLAGS) -Wall -Werror -Wextra -pthread -MMD -MP
CXXFLAGS := $(INC_FLAGS) $(EXTRA_FLAGS) -Wall -Werror -Wextra -std=c++2a -pthread -MMD -MP

.PHONY: default all $(APPNAME)

default all $(APPNAME): $(EXE)

# The final build step
$(EXE): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# Build step for C source
$(BUILD_DIR)%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)

endif # BUILD_DIR