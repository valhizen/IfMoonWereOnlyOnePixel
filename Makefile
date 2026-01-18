# Compiler and flags
CXX := clang++
STD := -std=c++20

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
EXTERNAL_DIR := external
GLAD_SRC_DIR := $(EXTERNAL_DIR)/GLAD/src
GLAD_INCLUDE_DIR := $(EXTERNAL_DIR)/GLAD/include

# ImGui directories - headers separate from source
IMGUI_SRC := $(SRC_DIR)/imgui
IMGUI_INCLUDE := $(INCLUDE_DIR)/imgui

# Include paths - headers from include/, source compiled from src/
CXXFLAGS := -I$(INCLUDE_DIR) \
            -I$(IMGUI_INCLUDE) \
            -I$(IMGUI_INCLUDE)/backends \
            -I$(GLAD_INCLUDE_DIR) \
            $(shell pkg-config --cflags glfw3 x11 )

LDFLAGS := $(shell pkg-config --libs glfw3 x11 ) \
           -lGL -lpthread -ldl -ltiff

# ImGui source files (from src/imgui/)
IMGUI_SRCS := $(IMGUI_SRC)/imgui.cpp \
              $(IMGUI_SRC)/imgui_demo.cpp \
              $(IMGUI_SRC)/imgui_draw.cpp \
              $(IMGUI_SRC)/imgui_tables.cpp \
              $(IMGUI_SRC)/imgui_widgets.cpp \
              $(IMGUI_SRC)/backends/imgui_impl_glfw.cpp \
              $(IMGUI_SRC)/backends/imgui_impl_opengl3.cpp

# Your source files - JUST ADD YOUR .cpp FILES HERE
MY_SRCS := $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/Application.cpp \
						$(SRC_DIR)/Planet.cpp \
						$(SRC_DIR)/Shader.cpp \
						$(SRC_DIR)/Camera.cpp \
						$(SRC_DIR)/readTexture.cpp \
						$(SRC_DIR)/Cockpit.cpp \
						$(SRC_DIR)/AudioSystem.cpp \
						$(SRC_DIR)/VideoPlayer.cpp \
						$(SRC_DIR)/AsteroidField.cpp



# GLAD source
GLAD_SRCS := $(GLAD_SRC_DIR)/glad.c

# Auto-generate object file paths
MY_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(MY_SRCS))
GLAD_OBJS := $(BUILD_DIR)/glad.o
IMGUI_OBJS := $(BUILD_DIR)/imgui/imgui.o \
              $(BUILD_DIR)/imgui/imgui_demo.o \
              $(BUILD_DIR)/imgui/imgui_draw.o \
              $(BUILD_DIR)/imgui/imgui_tables.o \
              $(BUILD_DIR)/imgui/imgui_widgets.o \
              $(BUILD_DIR)/imgui/imgui_impl_glfw.o \
              $(BUILD_DIR)/imgui/imgui_impl_opengl3.o

OBJS := $(MY_OBJS) $(GLAD_OBJS) $(IMGUI_OBJS)

TARGET := $(BUILD_DIR)/main

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CXX) $(STD) $(OBJS) -o $@ $(LDFLAGS)

# Pattern rule for your source files - automatically handles any .cpp in src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling: $<"
	$(CXX) $(STD) $(CXXFLAGS) -c $< -o $@

# GLAD files
$(BUILD_DIR)/glad.o: $(GLAD_SRC_DIR)/glad.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling GLAD: $<"
	clang -I$(GLAD_INCLUDE_DIR) $(shell pkg-config --cflags glfw3) -c $< -o $@

# ImGui core files
$(BUILD_DIR)/imgui/%.o: $(IMGUI_SRC)/%.cpp
	@mkdir -p $(BUILD_DIR)/imgui
	@echo "Compiling ImGui: $<"
	$(CXX) $(STD) $(CXXFLAGS) -c $< -o $@

# ImGui backend files
$(BUILD_DIR)/imgui/%.o: $(IMGUI_SRC)/backends/%.cpp
	@mkdir -p $(BUILD_DIR)/imgui
	@echo "Compiling ImGui backend: $<"
	$(CXX) $(STD) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
