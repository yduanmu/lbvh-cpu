CXX = g++
CXXFLAGS = -std=c++20 -Iinclude
SOURCE = $(wildcard src/*.cpp) $(wildcard src/lbvh/*.cpp) $(wildcard src/util/*.cpp)
OBJECTS = $(patsubst %.cpp,bin/%.o,$(SOURCE))
TARGET = bin/lbvh

default: $(TARGET)

bin:
	mkdir -p bin/src/lbvh bin/src/util

bin/%.o: %.cpp | bin
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf bin
