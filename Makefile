BUILD := build
BUILT := $(BUILD)/built
CMAKE := cmake
CMAKE_FLAGS := -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Debug \
  -DCMAKE_INSTALL_PREFIX:PATH=$(abspath $(BUILT)) \
  -DLLVM_EXTERNAL_CLANG_SOURCE_DIR:PATH=$(abspath clang)
NINJA := ninja

# Convenient setup stuff for getting LLVM and Clang ready and in place.
.PHONY: llvm

# Build LLVM using CMake and install it under the build/built/ prefix.
llvm:
	mkdir -p $(BUILD)/$@
	cd $(BUILD)/$@ ; $(CMAKE) $(CMAKE_FLAGS) ../../llvm
	cd $(BUILD)/$@ ; $(NINJA) install
