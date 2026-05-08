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
#include <cmath>

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
	// screen = origin + x * right + y * top
	float tx, ty;
	float m00, m10; // right vector
	float m01, m11; // top vector

	ImFont* font;
};

static CanvasContext* gContext = nullptr;

// -------------------------
// Default config
// -------------------------

CanvasConfig cvGetDefaultConfig(void)
{
	CanvasConfig cfg{};
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

	IM_ASSERT(window && "cvInit requires valid GLFWwindow");

	gContext = (CanvasContext*)calloc(1, sizeof(CanvasContext));
	IM_ASSERT(gContext && "Failed to allocate CanvasContext");

	gContext->window = window;
	gContext->backgroundColor = config.backgroundColor;
	gContext->fontSize = config.fontSize;
	gContext->pointRadius = config.pointRadius;
	gContext->lineThickness = config.lineThickness;
	gContext->showCoordSystem = false;
	gContext->showGrid = false;

	// Identity transform by default
	gContext->tx = 0.0f;
	gContext->ty = 0.0f;
	gContext->m00 = 1.0f;
	gContext->m10 = 0.0f;
	gContext->m01 = 0.0f;
	gContext->m11 = 1.0f;

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		free(gContext);
		gContext = nullptr;
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImFontConfig fontCfg{};
	gContext->font = io.Fonts->AddFontFromFileTTF(
		"assets/Roboto-Regular.ttf",
		config.fontSize,
		&fontCfg
	);

	if (!gContext->font)
		gContext->font = io.Fonts->AddFontDefault();

	unsigned char* pixels = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	ImGui_ImplOpenGL3_CreateFontsTexture();
}

void cvShutdown(void)
{
	if (!gContext)
	{
		IM_ASSERT(false && "cvShutdown: Canvas not initialized");
		return;
	}

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

	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
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

		for (int i = -5; i <= 5; ++i)
		{
			float x0, y0, x1, y1;

			cvCoordsToScreenSpace((float)i, -5.f, &x0, &y0);
			cvCoordsToScreenSpace((float)i, 5.f, &x1, &y1);
			dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col);

			cvCoordsToScreenSpace(-5.f, (float)i, &x0, &y0);
			cvCoordsToScreenSpace(5.f, (float)i, &x1, &y1);
			dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col);
		}
	}

	// ---------------- Axes ----------------
	if (gContext->showCoordSystem)
	{
		float x0, y0, x1, y1;

		cvCoordsToScreenSpace(-10.0f, 0.0f, &x0, &y0);
		cvCoordsToScreenSpace(10.0f, 0.0f, &x1, &y1);
		dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 0, 0, 255), 2.0f);

		cvCoordsToScreenSpace(0.0f, -10.0f, &x0, &y0);
		cvCoordsToScreenSpace(0.0f, 10.0f, &x1, &y1);
		dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 255, 0, 255), 2.0f);
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
		((c >> CV_COL32_R_SHIFT) & 0xFF) / 255.0f,
		((c >> CV_COL32_G_SHIFT) & 0xFF) / 255.0f,
		((c >> CV_COL32_B_SHIFT) & 0xFF) / 255.0f,
		((c >> CV_COL32_A_SHIFT) & 0xFF) / 255.0f
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
	float originX, float originY,
	float rightX, float rightY,
	float topX, float topY)
{
	IM_ASSERT(gContext && "cvSetCoordinateSystemFromScreenSpace: not initialized");

	gContext->tx = originX;
	gContext->ty = originY;

	gContext->m00 = rightX;
	gContext->m10 = rightY;

	gContext->m01 = topX;
	gContext->m11 = topY;
}

void cvCoordsToScreenSpace(float x, float y, float* sx, float* sy)
{
	IM_ASSERT(gContext);

	*sx = x * gContext->m00 + y * gContext->m01 + gContext->tx;
	*sy = x * gContext->m10 + y * gContext->m11 + gContext->ty;
}

void cvCoordsFromScreenSpace(float sx, float sy, float* x, float* y)
{
	IM_ASSERT(gContext);

	float dx = sx - gContext->tx;
	float dy = sy - gContext->ty;

	float det = gContext->m00 * gContext->m11 - gContext->m01 * gContext->m10;
	IM_ASSERT(fabs(det) > 1e-8f && "Coordinate system determinant is zero");

	*x = ( dx * gContext->m11 - dy * gContext->m01) / det;
	*y = (-dx * gContext->m10 + dy * gContext->m00) / det;
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

	ImGui::PushFont(gContext->font);
	dl->AddText(ImVec2(sx + 4, sy + 4), color, name);
	ImGui::PopFont();
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

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		w,
		h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
	if (id)
		glDeleteTextures(1, &id);
}

void cvAddTexture(float x, float y, CvTexture texture)
{
	float sx, sy;
	cvCoordsToScreenSpace(x, y, &sx, &sy);

	ImGui::GetBackgroundDrawList()->AddImage(
		(ImTextureID)(uintptr_t)texture.id,
		ImVec2(sx, sy),
		ImVec2(sx + texture.width, sy + texture.height)
	);
}
