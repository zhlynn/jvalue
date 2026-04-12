CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11
TARGET = jvalue
SRCDIR = src
BUILDDIR = .build
MAIN_SOURCES = $(SRCDIR)/json.cpp $(SRCDIR)/base64.cpp $(SRCDIR)/main.cpp
LIB_SOURCES = $(SRCDIR)/json.cpp $(SRCDIR)/base64.cpp
MAIN_OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(MAIN_SOURCES))
LIB_OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(LIB_SOURCES))

all: $(TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

benchmark: $(LIB_OBJECTS) $(BUILDDIR)/benchmark.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(LIB_OBJECTS) $(BUILDDIR)/test.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET) benchmark test

.PHONY: all clean
