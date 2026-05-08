CXX := g++
CC  := gcc
AR  := ar

LIB := libcanvas.a
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

INCLUDES := \
	-Iinclude \
	-Ithird_party/cimgui/imgui \
	-Ithird_party/cimgui/imgui/backends \
	-Ithird_party/cimgui/imgui/misc/cpp \
	-Ithird_party/cimgui

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra $(INCLUDES) \
	-DIMGUI_IMPL_OPENGL_LOADER_GLAD \
	-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS \
	-fPIC

CFLAGS := -O2 -Wall -Wextra $(INCLUDES) -fPIC

# =========================
# IMGUI CORE (MUST STAY TOGETHER)
# =========================

IMGUI := \
	third_party/cimgui/imgui/imgui.cpp \
	third_party/cimgui/imgui/imgui_draw.cpp \
	third_party/cimgui/imgui/imgui_tables.cpp \
	third_party/cimgui/imgui/imgui_widgets.cpp \
	third_party/cimgui/imgui/imgui_demo.cpp \
	third_party/cimgui/imgui/misc/cpp/imgui_stdlib.cpp \
	third_party/cimgui/imgui/backends/imgui_impl_glfw.cpp \
	third_party/cimgui/imgui/backends/imgui_impl_opengl3.cpp

# =========================
# CIMGUI (depends on ImGui)
# =========================

CIMGUI := third_party/cimgui/cimgui.cpp

# =========================
# ENGINE
# =========================

ENGINE_CPP := src/canvas.cpp
ENGINE_C := src/glad.c

# =========================
# ALL SOURCES (CRITICAL ORDER)
# =========================

SOURCES_CPP := \
	$(IMGUI) \
	$(CIMGUI) \
	$(ENGINE_CPP)

SOURCES_C := $(ENGINE_C)

OBJS_CPP := $(SOURCES_CPP:%.cpp=$(OBJ_DIR)/%.o)
OBJS_C   := $(SOURCES_C:%.c=$(OBJ_DIR)/%.o)

OBJS := $(OBJS_CPP) $(OBJS_C)

# =========================
# BUILD RULES
# =========================

all: $(LIB)

$(LIB): $(OBJS)
	@echo "ARCHIVING $@"
	@rm -f $@
	@$(AR) rcs $@ $^
	@ranlib $@ || true

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB)

re: clean all

.PHONY: all clean re