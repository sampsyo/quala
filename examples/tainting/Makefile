include ../../common.mk

SOURCES := TaintTracking.cpp
HEADERS := ../../TypeAnnotations.h
TARGET := TaintTracking.$(LIBEXT)

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
.PHONY: test dump smoke
test: $(TARGET)
	$(BUILD)/llvm/bin/llvm-lit -v test
dump: $(TARGET)
	cd test ; ../ttclang -S -emit-llvm -o - ok.c
smoke: $(TARGET)
	cd test ; ../ttclang -S -emit-llvm -o /dev/null endorsement.c
