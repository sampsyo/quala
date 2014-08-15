BUILD := build
BUILT := $(BUILD)/built
CMAKE := cmake
CMAKE_FLAGS := -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Debug \
  -DCMAKE_INSTALL_PREFIX:PATH=$(abspath $(BUILT))
NINJA := ninja

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
