#CC  = arm-poky-linux-gnueabi-gcc
#CXX = arm-poky-linux-gnueabi-g++
#STRIP = arm-poky-linux-gnueabi-strip

BUILD ?=
BUILD_DIR = ../build/${BUILD}
OBJ_DIR   = ../bin/${BUILD}
INCL_DIR  = ../include
SRC_DIR   = ./
LIB_DIR   = ../lib

EXT = cpp
EXEC = $(BUILD_DIR)/exec
SRCS = $(wildcard $(SRC_DIR)/*.${EXT})
OBJS = $(patsubst $(SRC_DIR)/%.${EXT},$(OBJ_DIR)/%.${EXT}.o,$(SRCS))

CXXFLAGS.		= -I$(INCL_DIR) -Wall -g
CXXFLAGS.debug	=  $(CXXFLAGS.) -DDEBUG
CFLAGS.			= -I$(INCL_DIR) -Wall -g
CFLAGS.debug	=  $(CFLAGS.) -DDEBUG

LDLIBS			= # Load libraries

COMPILER.c		= $(CC)
COMPILER.cpp	= $(CXX)

.PHONY: all run clean

all: $(EXEC)

$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(COMPILER.$(EXT)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS.$(BUILD)) -c -o $@ $<

$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS.$(BUILD)) -c -o $@ $<

$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

run: $(EXEC)
	$<

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)