BUILD := build
BUILT := $(BUILD)/built
CMAKE := cmake
CMAKE_FLAGS := -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Debug \
  -DCMAKE_INSTALL_PREFIX:PATH=$(abspath $(BUILT)) \
  -DLLVM_EXTERNAL_CLANG_SOURCE_DIR:PATH=$(abspath clang)
NINJA := ninja

# On OS X, build Clang with a built-in search path for this platform. This
# feels a little dirty, since the user can hypothetically change the XCode
# "platform" later on, but I can't find a way to tell Clang to just look for
# *the current platform*. This is the next best thing.
ifeq ($(shell uname -s),Darwin)
CMAKE_FLAGS += \
  -DC_INCLUDE_DIRS:STRING=$(shell xcrun --show-sdk-path)/usr/include
endif

# Convenient setup stuff for getting LLVM and Clang ready and in place.
.PHONY: llvm

# Build LLVM using CMake and install it under the build/built/ prefix.
llvm:
	mkdir -p $(BUILD)/$@
	cd $(BUILD)/$@ ; $(CMAKE) $(CMAKE_FLAGS) ../../llvm
	cd $(BUILD)/$@ ; $(NINJA) install
