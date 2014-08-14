BUILD := build
BUILT := $(BUILD)/built
BUILT_ABS := $(shell pwd)/$(BUILT)
CMAKE := cmake
CMAKE_FLAGS := -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$(BUILT_ABS)
NINJA := ninja
LLVM_CONFIG := $(BUILT)/bin/llvm-config

# Get LLVM build parameters from its llvm-config program.
export LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags) -fno-rtti
export LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
export LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs --system-libs)
export CLANG_LIBS := \
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
export PLUGIN_LDFLAGS

# Platform-specific library suffix.
ifeq ($(shell uname -s),Darwin)
		LIBEXT := dylib
else
		LIBEXT := so
endif
export LIBEXT

# Build the example.
.PHONY: tainting
tainting:
	make -C examples/$@

# Convenient setup stuff for getting LLVM and Clang ready and in place.
.PHONY: llvm

# Build LLVM using CMake and install it under the build/built/ prefix.
llvm: llvm/tools/clang
	mkdir -p $(BUILD)/$@
	cd $(BUILD)/$@ ; $(CMAKE) $(CMAKE_FLAGS) ../../llvm
	cd $(BUILD)/$@ ; $(NINJA) install

# Symlink to clang source tree.
llvm/tools/clang:
	cd llvm/tools ; ln -s ../../clang .
