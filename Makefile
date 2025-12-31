BUILD_DIR := build
ANALYZE_DIR := build-analyze
ENGINE_NAME := quark

UNAME_S := $(shell uname -s)

VCPKG_TARGET_TRIPLET ?=
VCPKG_HOST_TRIPLET ?=
CMAKE_OSX_ARCHITECTURES ?=

ifeq ($(UNAME_S),Darwin)
  ifeq ($(strip $(VCPKG_TARGET_TRIPLET)),)
    VCPKG_TARGET_TRIPLET := arm64-osx
  endif
  ifeq ($(strip $(VCPKG_HOST_TRIPLET)),)
    VCPKG_HOST_TRIPLET := arm64-osx
  endif
  ifeq ($(strip $(CMAKE_OSX_ARCHITECTURES)),)
    CMAKE_OSX_ARCHITECTURES := arm64
  endif
endif

CMAKE_FLAGS := -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/vcpkg/scripts/buildsystems/vcpkg.cmake \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ifneq ($(strip $(VCPKG_TARGET_TRIPLET)),)
CMAKE_FLAGS += -DVCPKG_TARGET_TRIPLET=$(VCPKG_TARGET_TRIPLET)
endif
ifneq ($(strip $(VCPKG_HOST_TRIPLET)),)
CMAKE_FLAGS += -DVCPKG_HOST_TRIPLET=$(VCPKG_HOST_TRIPLET)
endif
ifneq ($(strip $(CMAKE_OSX_ARCHITECTURES)),)
CMAKE_FLAGS += -DCMAKE_OSX_ARCHITECTURES=$(CMAKE_OSX_ARCHITECTURES)
endif

.PHONY: configure configure-analyze analyze tidy format format-check build run clean clean-analyze vcpkg-setup shaders

configure: vcpkg-setup

# PIN TO A HASH IN A BETTER WAY THAN THIS?
VCPKG_COMMIT := 11bbc873e00e9e58d4e9dffb30b7a5493a030e0b

vcpkg-setup:
	@echo "Setting up vcpkg at commit ${VCPKG_COMMIT}..."

	@if [ ! -d vcpkg ]; then \
		git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg; \
		cd vcpkg && git fetch origin ${VCPKG_COMMIT} && git checkout ${VCPKG_COMMIT}; \
		./bootstrap-vcpkg.sh; \
	else \
		echo "vcpkg already exists, doing nothing"; \
	fi


configure:
	VCPKG_DISABLE_METRICS=1 \
	VCPKG_DEFAULT_TRIPLET=$(VCPKG_TARGET_TRIPLET) \
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: shaders configure
	cmake --build $(BUILD_DIR)

GLSLC ?= glslc
SHADERS_OUT_DIR := shaders/bin

shaders:
	@mkdir -p $(SHADERS_OUT_DIR)
	@$(GLSLC) src/backend/shaders/shader.vert -o $(SHADERS_OUT_DIR)/shader.vert.spv
	@$(GLSLC) src/backend/shaders/shader.frag -o $(SHADERS_OUT_DIR)/shader.frag.spv

run: build
	./$(BUILD_DIR)/src/$(ENGINE_NAME)

configure-analyze:
	VCPKG_DISABLE_METRICS=1 \
	VCPKG_DEFAULT_TRIPLET=$(VCPKG_TARGET_TRIPLET) \
	scan-build --status-bugs cmake -S . -B $(ANALYZE_DIR) $(CMAKE_FLAGS)

analyze: configure-analyze
	scan-build --status-bugs cmake --build $(ANALYZE_DIR)

tidy: configure
	clang-tidy -p $(BUILD_DIR) src/**/*.cpp

format:
	find . \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.inl' \) \
	  -not -path './build*/*' -not -path './_deps/*' -not -path './vcpkg/*' \
	  -print0 | xargs -0 clang-format -i

format-check:
	find . \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.inl' \) \
	  -not -path './build*/*' -not -path './_deps/*' -not -path './vcpkg/*' \
	  -print0 | xargs -0 clang-format -n -Werror

clean:
	rm -rf $(BUILD_DIR)

clean-analyze:
	rm -rf $(ANALYZE_DIR)
