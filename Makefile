TARGET = LiveStreamServer

BUILD_DIR = build
BINARY_DIR = .

DEBUG = 1

CC = gcc
CXX = g++
CFLAGS =
CXXFLAGS = -std=c++17

LDFLAGS = -pthread 

SRC = \
src/capture.cpp \
src/main.cpp 


INC = -I/usr/local/include

LIBS =  -L/usr/local/lib -ldatachannel


ifeq ($(DEBUG), 1)
	CFLAGS += -g 
	CXXFLAGS += -g 
	LDFLAGS += -g
endif


OBJ = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(notdir $(basename $(SRC)))))
vpath %.c $(sort $(dir $(SRC)))
vpath %.cpp $(sort $(dir $(SRC)))

.PHONY: all clean print

all: $(BINARY_DIR)/$(TARGET)

$(BUILD_DIR)/%.o : %.c Makefile | $(BUILD_DIR)
	$(CC) -MM -MF $(BUILD_DIR)/$(notdir $(@:.o=.d)) $<
	$(CC) -c -Wall $(CFLAGS) $(INC) $< -o $@ 

$(BUILD_DIR)/%.o : %.cpp Makefile | $(BUILD_DIR)
	$(CXX) -MM -MF $(BUILD_DIR)/$(notdir $(@:.o=.d)) $<
	$(CXX) -c -Wall $(CXXFLAGS) $(INC) $< -o $@ 

$(BINARY_DIR)/$(TARGET): $(OBJ) Makefile
	$(CXX) $(LDFLAGS) $(LIBS) $(OBJ) -o $@ $(LIBS)

$(BUILD_DIR): 
	mkdir $@

clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) $(BINARY_DIR)/$(TARGET)

print:
	$(info sourcefiles: $(SRC))
	$(info objects: $(OBJ))
	$(info output: $(TARGET))

-include $(wildcard $(BUILD_DIR)/*.d)