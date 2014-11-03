include ../../common.mk

CHECKER_SOURCES := Nullness.cpp
PASS_SOURCES := NullChecks.cpp ../../AnnotationInfo.cpp
HEADERS := ../../TypeAnnotations.h
CHECKER_TARGET := Nullness.$(LIBEXT)
PASS_TARGET := NullChecks.$(LIBEXT)

CHECKER_OBJS := $(CHECKER_SOURCES:%.cpp=%.o)
PASS_OBJS := $(PASS_SOURCES:%.cpp=%.o)

CXXFLAGS += -I../..

.PHONY: all
all: $(CHECKER_TARGET) $(PASS_TARGET)

# Build the Clang plugin module.
$(CHECKER_TARGET): $(CHECKER_OBJS)
	$(CXX) $(PLUGIN_LDFLAGS) $(CXXFLAGS) \
		$(LLVM_CXXFLAGS) $(LLVM_LDFLAGS) \
		-o $@ $^

# Build the LLVM pass module.
$(PASS_TARGET): $(PASS_OBJS)
	$(CXX) $(PLUGIN_LDFLAGS) $(CXXFLAGS) \
		$(LLVM_CXXFLAGS) $(LLVM_LDFLAGS) \
		-o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $(LLVM_CXXFLAGS) \
		-o $@ $<

.PHONY: clean
clean:
	rm -rf $(CHECKER_TARGET) $(CHECKER_OBJS) $(PASS_TARGET) $(PASS_OBJS)

# Testing stuff.
.PHONY: test smoke
test: $(TARGET)
	$(BUILD)/llvm/bin/llvm-lit -v test
smoke: $(TARGET)
	./nullness-c++ -S -emit-llvm -o /dev/null test/simple.cpp -v
