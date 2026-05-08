#include <canvas.h>

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cimgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// -------------------------
// Internal context
// -------------------------

struct CanvasContext
{
	uint32_t backgroundColor;
	GLFWwindow* window;

	float fontSize;
	float pointRadius;
	float lineThickness;

	bool showCoordSystem;
	bool showGrid;

	// Affine transform:
	// screen.x = x*m00 + tx + y*m01
	// screen.y = x*m10 + ty + y*m11
	float tx, ty;
	float m00, m10;
	float m01, m11;

	ImFont* font;
};

static CanvasContext* gContext = nullptr;

// -------------------------
// Default config
// -------------------------

CanvasConfig cvGetDefaultConfig(void)
{
	CanvasConfig cfg;
	cfg.backgroundColor = CV_COL32(30, 30, 30, 255);
	cfg.fontSize = 16.0f;
	cfg.pointRadius = 4.0f;
	cfg.lineThickness = 1.5f;
	return cfg;
}

// -------------------------
// Init / Shutdown
// -------------------------

void cvInit(GLFWwindow* window, CanvasConfig config)
{
	if (gContext)
	{
		IM_ASSERT(false && "cvInit must only be called once");
		return;
	}

	gContext = (CanvasContext*)calloc(1, sizeof(CanvasContext));

	gContext->window = window;
	gContext->backgroundColor = config.backgroundColor;
	gContext->fontSize = config.fontSize;
	gContext->pointRadius = config.pointRadius;
	gContext->lineThickness = config.lineThickness;

	// default coordinate system = identity
	gContext->tx = 0.0f;
	gContext->ty = 0.0f;
	gContext->m00 = 1.0f;
	gContext->m11 = 1.0f;
	gContext->m01 = 0.0f;
	gContext->m10 = 0.0f;

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	gContext->font = io.Fonts->AddFontDefault();
	ImFontConfig cfg;
	gContext->font = io.Fonts->AddFontFromFileTTF(
		"assets/Roboto-Regular.ttf",
		config.fontSize,
		&cfg
	);
}

void cvShutdown(void)
{
	if (!gContext)
		IM_ASSERT(false && "cvShutdown: Canvas not initialized");

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	free(gContext);
	gContext = nullptr;
}

// -------------------------
// Frame lifecycle
// -------------------------

void cvNewFrame(void)
{
	IM_ASSERT(gContext && "cvNewFrame: not initialized");

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Canvas");

	ImGui::Checkbox("Display coordinate system", &gContext->showCoordSystem);
	ImGui::Checkbox("Draw grid", &gContext->showGrid);

	ImGui::End();

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// ---------------- Grid ----------------
	if (gContext->showGrid)
	{
		ImU32 col = IM_COL32(200, 200, 200, 60);

		for (int i = -5; i <= 5; i++)
		{
			float x0, y0, x1, y1;

			cvCoordsToScreenSpace(i, -5, &x0, &y0);
			cvCoordsToScreenSpace(i,  5, &x1, &y1);
			dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col);

			cvCoordsToScreenSpace(-5, i, &x0, &y0);
			cvCoordsToScreenSpace( 5, i, &x1, &y1);
			dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col);
		}
	}

	// ---------------- Axes ----------------
	if (gContext->showCoordSystem)
	{
		float x0, y0, x1, y1;

		cvCoordsToScreenSpace(-10, 0, &x0, &y0);
		cvCoordsToScreenSpace( 10, 0, &x1, &y1);
		dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 0, 0, 255));

		cvCoordsToScreenSpace(0, -10, &x0, &y0);
		cvCoordsToScreenSpace(0,  10, &x1, &y1);
		dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 255, 0, 255));
	}
}

void cvEndFrame(void)
{
	IM_ASSERT(gContext && "cvEndFrame: not initialized");

	ImGui::Render();

	int w, h;
	glfwGetFramebufferSize(gContext->window, &w, &h);
	glViewport(0, 0, w, h);

	uint32_t c = gContext->backgroundColor;
	glClearColor(
		((c >> 0) & 0xFF) / 255.0f,
		((c >> 8) & 0xFF) / 255.0f,
		((c >> 16) & 0xFF) / 255.0f,
		((c >> 24) & 0xFF) / 255.0f
	);

	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// -------------------------
// Config access
// -------------------------

CanvasConfig* cvGetConfig(void)
{
	IM_ASSERT(gContext && "cvGetConfig: not initialized");

	static CanvasConfig cfg;
	cfg.backgroundColor = gContext->backgroundColor;
	cfg.fontSize = gContext->fontSize;
	cfg.pointRadius = gContext->pointRadius;
	cfg.lineThickness = gContext->lineThickness;
	return &cfg;
}

// -------------------------
// Coordinate transforms
// -------------------------

void cvSetCoordinateSystemFromScreenSpace(
	float ox, float oy,
	float rx, float ry,
	float tx, float ty)
{
	gContext->tx = ox;
	gContext->ty = oy;
	gContext->m00 = rx;
	gContext->m11 = ry;
	gContext->m01 = tx;
	gContext->m10 = ty;
}

void cvCoordsToScreenSpace(float x, float y, float* sx, float* sy)
{
	*sx = x * gContext->m00 + gContext->tx + y * gContext->m01;
	*sy = x * gContext->m10 + gContext->ty + y * gContext->m11;
}

void cvCoordsFromScreenSpace(float sx, float sy, float* x, float* y)
{
	float det = gContext->m10 * gContext->m01 - gContext->m00 * gContext->m11;

	float dx = sx - gContext->tx;
	float dy = sy - gContext->ty;

	*x = (dx * gContext->m11 - dy * gContext->m01) / det;
	*y = (dy * gContext->m00 - dx * gContext->m10) / det;
}

// -------------------------
// Drawing primitives
// -------------------------

void cvAddPoint(float x, float y, unsigned int color)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImGui::GetBackgroundDrawList()->AddCircleFilled(
		ImVec2(sx, sy),
		gContext->pointRadius,
		color
	);
}

void cvAddNamedPoint(float x, float y, unsigned int color, const char* name)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	dl->AddCircleFilled(ImVec2(sx, sy), gContext->pointRadius, color);
	dl->AddText(ImVec2(sx + 4, sy + 4), color, name);
}

void cvAddLine(float x0, float y0, float x1, float y1, unsigned int color)
{
	float sx0, sy0, sx1, sy1;
	cvCoordsToScreenSpace(x0, y0, &sx0, &sy0);
	cvCoordsToScreenSpace(x1, y1, &sx1, &sy1);

	ImGui::GetBackgroundDrawList()->AddLine(
		ImVec2(sx0, sy0),
		ImVec2(sx1, sy1),
		color,
		gContext->lineThickness
	);
}

void cvAddText(float x, float y, unsigned int color, const char* text)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImGui::PushFont(gContext->font);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(sx, sy), color, text);
	ImGui::PopFont();
}

void cvAddFormattedText(float x, float y, unsigned int color, const char* fmt, ...)
{
	char buf[2048];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	cvAddText(x, y, color, buf);
}

// -------------------------
// Path API
// -------------------------

void cvPathLineTo(float x, float y)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImGui::GetBackgroundDrawList()->PathLineTo(ImVec2(sx, sy));
}

void cvPathStroke(unsigned int color, int closed)
{
	ImGui::GetBackgroundDrawList()->PathStroke(color, closed, gContext->lineThickness);
}

void cvPathFill(unsigned int color)
{
	ImGui::GetBackgroundDrawList()->PathFillConvex(color);
}

// -------------------------
// Texture
// -------------------------

CvTexture cvLoadTexture(const char* path)
{
	CvTexture tex{};
	if (!gContext)
		return tex;

	int w, h, channels;
	unsigned char* data = stbi_load(path, &w, &h, &channels, 4);

	if (!data)
	{
		fprintf(stderr, "Failed to load texture: %s\n", path);
		return tex;
	}

	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	tex.id = (CvTextureID)(uintptr_t)id;
	tex.width = w;
	tex.height = h;
	return tex;
}

void cvUnloadTexture(CvTexture texture)
{
	GLuint id = (GLuint)(uintptr_t)texture.id;
	glDeleteTextures(1, &id);
}

void cvAddTexture(float x, float y, CvTexture texture)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImVec2 size((float)texture.width, (float)texture.height);

	ImGui::GetBackgroundDrawList()->AddImage(
		(ImTextureID)(uintptr_t)texture.id,
		ImVec2(sx, sy),
		ImVec2(sx + size.x, sy + size.y)
	);
}