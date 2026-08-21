CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -Isrc
TARGET = jvalue
BUILDDIR = .build

all: $(TARGET)

$(TARGET): $(BUILDDIR)/json.o $(BUILDDIR)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(BUILDDIR)/test

benchmark: $(BUILDDIR)/benchmark

$(BUILDDIR)/test: $(BUILDDIR)/json.o $(BUILDDIR)/test.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR)/benchmark: $(BUILDDIR)/json.o $(BUILDDIR)/benchmark.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR)/json.o: src/json.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/main.o: main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

.PHONY: all clean test benchmark
