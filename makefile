CXX = mpic++
CXXFLAGS = -std=c++20
SOURCE = $(wildcard *.cpp) $(wildcard lbvh/*.cpp) $(wildcard util/*.cpp)
OBJECTS = $(SOURCE:.cpp=.o)
TARGET = lbvh

default: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
