CXX   = arm-poky-linux-gnueabi-g++
STRIP = arm-poky-linux-gnueabi-strip

BUILD ?=
PROJ_DIR  = ..
BUILD_DIR = $(PROJ_DIR)/build/${BUILD}
OBJ_DIR   = $(PROJ_DIR)/bin/${BUILD}
INCL_DIR  = $(PROJ_DIR)/include
SRC_DIR   = $(PROJ_DIR)/src
LIB_DIR   = $(PROJ_DIR)/lib

EXEC = $(BUILD_DIR)/exec
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

CXXFLAGS.      = -I$(INCL_DIR) -Wall -O2 -march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4
CXXFLAGS.debug =  $(CXXFLAGS.) -g -DDEBUG
LDLIBS = -lpthread # Load libraries

.PHONY: all run clean

all: $(EXEC)

$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(STRIP) $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS.$(BUILD)) -c -o $@ $<

$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

run: $(EXEC)
	$<

send:
	scp -r $(PROJ_DIR)/{src,include} "matrix752:~/VernierRead/"

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)