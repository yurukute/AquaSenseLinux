CXX = arm-poky-linux-gnueabi-g++
STRIP = arm-poky-linux-gnueabi-strip

BUILD ?=
BUILD_DIR = ../build/${BUILD}
OBJ_DIR   = ../bin/${BUILD}
INCL_DIR  = ../include
SRC_DIR   = ./
LIB_DIR   = ../lib

EXEC = $(BUILD_DIR)/exec
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

CXXFLAGS.      = -I$(INCL_DIR) -Wall
CXXFLAGS.debug =  $(CXXFLAGS.) -g -DDEBUG
LDLIBS = -lpthread # Load libraries

.PHONY: all run clean

all: $(EXEC)

$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS.$(BUILD)) -c -o $@ $<

$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

run: $(EXEC)
	$<

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)