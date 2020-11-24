CXX      := -g++ -std=c++17
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -Wno-deprecated-copy -Wno-ignored-attributes -O0 -g
LDFLAGS  := -L/usr/lib -lstdc++ -lm -lssl -lcrypto
BUILD	 := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := client server
INCLUDE  := -Iinclude/

SRC := $(wildcard src/*.cpp)
OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

.PHONY: all build clean client server

build:
	@mkdir -p $(OBJ_DIR)

client: $(filter-out $(OBJ_DIR)/src/server.o, $(OBJECTS))
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

server: $(filter-out $(OBJ_DIR)/src/client.o, $(OBJECTS))
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	@rm -rvf $(OBJ_DIR)/*
