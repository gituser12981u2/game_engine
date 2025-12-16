BUILD_DIR := build

configure:
	cmake -S . -B $(BUILD_DIR) -G Ninja

tidy: configure
	clang-tidy -p $(BUILD_DIR) \
		$(shell find src -name '*.cpp' -o -name '*.hpp')

run: build 
	./$(BUILD_DIR)/src/engine_app

clean:
	rm -rf $(BUILD_DIR)
