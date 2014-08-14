HERE := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD := $(HERE)/build
BUILT := $(BUILD)/built
CMAKE := cmake
CMAKE_FLAGS := -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$(realpath $(BUILT_ABS))
NINJA := ninja
LLVM_CONFIG := $(BUILT)/bin/llvm-config

# Get LLVM build parameters from its llvm-config program.
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags) -fno-rtti
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs --system-libs)
CLANG_LIBS := \
	-lclangAST \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangFrontendTool \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangEdit \
	-lclangASTMatchers \
	-lclangRewriteCore \
	-lclangRewriteFrontend \
	-lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangSerialization \
	-lclangTooling

# On OS X, you need to tell the linker that undefined symbols will be looked
# up at runtime.
PLUGIN_LDFLAGS := -shared
ifeq ($(shell uname -s),Darwin)
	PLUGIN_LDFLAGS += -Wl,-undefined,dynamic_lookup
endif

# Platform-specific library suffix.
ifeq ($(shell uname -s),Darwin)
		LIBEXT := dylib
else
		LIBEXT := so
endif
