BUILD_DIR := build

configure:
	cmake -S . -B $(BUILD_DIR) -G Ninja

	mkdir -p shaders/bin

	glslc src/backend/shaders/shader.frag -o shaders/bin/shader.frag.spv

	glslc src/backend/shaders/shader.vert -o shaders/bin/shader.vert.spv

.PHONY: configure build tidy run clean

build: configure
	cmake --build $(BUILD_DIR)

tidy: configure
	clang-tidy -p $(BUILD_DIR) \
		$(shell find src -name '*.cpp' -o -name '*.hpp')

run: build
	./$(BUILD_DIR)/src/engine_app

clean:
	rm -rf $(BUILD_DIR)
