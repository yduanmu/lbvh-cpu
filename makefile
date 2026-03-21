CXX = mpic++
CXXFLAGS = -std=c++20
SOURCE = $(wildcard *.cpp)
OBJECTS = $(SOURCE:.cpp=.o)
TARGET = lbvh

default: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

sssp: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
