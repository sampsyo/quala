include ../../common.mk

SOURCES := Nullness.cpp
HEADERS := ../../TypeAnnotations.h
TARGET := Nullness.$(LIBEXT)

OBJS := $(SOURCES:%.cpp=%.o)

CXXFLAGS += -I../..

$(TARGET): $(OBJS)
	$(CXX) $(PLUGIN_LDFLAGS) $(CXXFLAGS) \
		$(LLVM_CXXFLAGS) $(LLVM_LDFLAGS) \
		-o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(LLVM_CXXFLAGS) \
		-o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS)

# Testing stuff.
.PHONY: test smoke
test: $(TARGET)
	$(BUILD)/llvm/bin/llvm-lit -v test
smoke: $(TARGET)
	./nullness-c++ -S -emit-llvm -o /dev/null test/simple.cpp -v
