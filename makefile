CXX = mpic++
CXXFLAGS = -std=c++20 -Iinclude
SOURCE = $(wildcard src/*.cpp) $(wildcard src/lbvh/*.cpp) $(wildcard src/util/*.cpp)
OBJECTS = $(SOURCE:.cpp=.o)
TARGET = lbvh

default: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
