#include <canvas.h>

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// -------------------------
// Internal context
// -------------------------

struct CanvasContext
{
	CanvasConfig	config;
	GLFWwindow*		window;
	bool			showCoordSystem;
	bool			showGrid;
	
	// Affine coordinate system:
	// screen = origin + x * right + y * up
	ImVec2			origin;
	ImVec2			right;
	ImVec2			up;
	
	ImFont*			font;
};

static CanvasContext* gContext = nullptr;

// -------------------------
// Internal helpers
// -------------------------

static inline void cvAssertInitialized(const char* fn)
{
	IM_ASSERT(gContext && fn);
}

static ImVec2 ScreenFromCoords(float x, float y)
{
	cvAssertInitialized("Canvas was not initialized");

	return ImVec2(
		gContext->origin.x + x * gContext->right.x + y * gContext->up.x,
		gContext->origin.y + x * gContext->right.y + y * gContext->up.y
	);
}

static ImVec2 CoordsFromScreen(float sx, float sy)
{
	cvAssertInitialized("Canvas was not initialized");

	const float ox = gContext->origin.x;
	const float oy = gContext->origin.y;
	const float rx = gContext->right.x;
	const float ry = gContext->right.y;
	const float ux = gContext->up.x;
	const float uy = gContext->up.y;

	const float det = ux * ry - uy * rx;
	IM_ASSERT(det != 0.0f && "Coordinate system matrix is singular");

	const float inv = 1.0f / det;

	const float x = (-sx * uy + sy * ux + ox * uy - oy * ux) * inv;
	const float y = ( sx * ry - sy * rx - ox * ry + oy * rx) * inv;

	return ImVec2(x, y);
}

// -------------------------
// Default config
// -------------------------

extern "C" CanvasConfig cvGetDefaultConfig(void)
{
	CanvasConfig cfg{};
	cfg.backgroundColor		= CV_COL32(30, 30, 30, 255);
	cfg.fontSize			= 30.0f;
	cfg.pointRadius			= 3.0f;
	cfg.lineThickness		= 3.0f;
	cfg.hideInternalWindow = 0;
	return cfg;
}

// -------------------------
// Init / Shutdown
// -------------------------

extern "C" void cvInit(GLFWwindow* window, const CanvasConfig config)
{
	IM_ASSERT(gContext == nullptr && "cvInit(): cvInit must be called once at startup.");
	IM_ASSERT(window && "cvInit(): valid GLFWwindow required.");

	gContext = static_cast<CanvasContext*>(calloc(1, sizeof(CanvasContext)));
	IM_ASSERT(gContext && "Failed to allocate CanvasContext");

	gContext->window = window;
	gContext->config = config;
	// Default coordinate system = screen space
	gContext->origin = ImVec2(0.0f, 0.0f);
	gContext->right  = ImVec2(1.0f, 0.0f);
	gContext->up     = ImVec2(0.0f, 1.0f);
	gContext->showCoordSystem = false;
	gContext->showGrid = false;

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		free(gContext);
		gContext = nullptr;
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	io.Fonts->AddFontDefault();

	gContext->font = io.Fonts->AddFontFromFileTTF(
		"assets/Roboto-Regular.ttf",
		gContext->config.fontSize
	);

	if (!gContext->font)
		gContext->font = io.FontDefault;

	ImGui_ImplOpenGL3_CreateFontsTexture();
}

extern "C" void cvShutdown(void)
{
	cvAssertInitialized("cvShutdown(): Canvas was not initialized");

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	free(gContext);
	gContext = nullptr;
}

// -------------------------
// Frame lifecycle
// -------------------------

extern "C" void cvNewFrame(void)
{
	cvAssertInitialized("cvNewFrame(): Canvas was not initialized");

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (!gContext->config.hideInternalWindow)
	{
		ImGui::SetNextWindowPos(ImVec2(10.f, 10.f), ImGuiCond_Always);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize;

		if (ImGui::Begin("Canvas", nullptr, flags))
		{
			ImGui::Checkbox("Display coordinate system", &gContext->showCoordSystem);
			ImGui::Checkbox("Draw grid", &gContext->showGrid);
		}
		ImGui::End();
	}

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// ---------------- Grid ----------------
	if (gContext->showGrid)
	{
		constexpr int halfWidth  = 5;
		constexpr int halfHeight = 5;
		const ImU32 color = IM_COL32(255, 255, 255, 64);

		for (int y = -halfHeight; y <= halfHeight; ++y)
			dl->AddLine(
				ScreenFromCoords((float)-halfWidth, (float)y),
				ScreenFromCoords((float)+halfWidth, (float)y),
				color,
				1.0f
			);

		for (int x = -halfWidth; x <= halfWidth; ++x)
			dl->AddLine(
				ScreenFromCoords((float)x, (float)-halfHeight),
				ScreenFromCoords((float)x, (float)+halfHeight),
				color,
				1.0f
			);
	}

	// ---------------- Coordinate axes ----------------
	if (gContext->showCoordSystem)
	{
		const ImVec2 origin = ScreenFromCoords(0.f, 0.f);
		const ImVec2 xAxis  = ScreenFromCoords(1.f, 0.f);
		const ImVec2 yAxis  = ScreenFromCoords(0.f, 1.f);

		dl->AddLine(origin, xAxis, IM_COL32(255, 0, 0, 255), 1.0f);
		dl->AddLine(origin, yAxis, IM_COL32(0, 255, 0, 255), 1.0f);
	}
}

extern "C" void cvEndFrame(void)
{
	cvAssertInitialized("cvEndFrame(): Canvas was not initialized");

	ImGui::Render();

	int w, h;
	glfwGetFramebufferSize(gContext->window, &w, &h);
	glViewport(0, 0, w, h);

	ImVec4 clearColor = ImColor(gContext->config.backgroundColor);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// -------------------------
// Config access
// -------------------------

extern "C" CanvasConfig* cvGetConfig(void)
{
	cvAssertInitialized("cvGetConfig(): Canvas was not initialized");
	return &gContext->config;
}

// -------------------------
// Coordinate system API
// -------------------------

extern "C" void cvSetCoordinateSystemFromScreenSpace(
	float originX,	float originY,
	float rightX,	float rightY,
	float upX,		float upY)
{
	cvAssertInitialized("cvSetCoordinateSystemFromScreenSpace(): Canvas was not initialized");

	gContext->origin = ImVec2(originX, originY);
	gContext->right  = ImVec2(rightX, rightY);
	gContext->up     = ImVec2(upX, upY);
}

extern "C" void cvCoordsToScreenSpace(float x, float y, float* sx, float* sy)
{
	cvAssertInitialized("cvCoordsToScreenSpace(): Canvas was not initialized");
	IM_ASSERT(sx && sy);

	ImVec2 p = ScreenFromCoords(x, y);
	*sx = p.x;
	*sy = p.y;
}

extern "C" void cvCoordsFromScreenSpace(float sx, float sy, float* x, float* y)
{
	cvAssertInitialized("cvCoordsFromScreenSpace(): Canvas was not initialized");
	IM_ASSERT(x && y);

	ImVec2 p = CoordsFromScreen(sx, sy);
	*x = p.x;
	*y = p.y;
}

// -------------------------
// Drawing primitives
// -------------------------

extern "C" void cvAddPoint(float x, float y, unsigned int color)
{
	cvAssertInitialized("cvAddPoint(): Canvas was not initialized");

	ImGui::GetBackgroundDrawList()->AddCircleFilled(
		ScreenFromCoords(x, y),
		gContext->config.pointRadius,
		color
	);
}

extern "C" void cvAddNamedPoint(float x, float y, unsigned int color, const char* name)
{
	cvAssertInitialized("cvAddNamedPoint(): Canvas was not initialized");

	ImVec2 p = ScreenFromCoords(x, y);
	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	dl->AddCircleFilled(p, gContext->config.pointRadius, color);

	ImGui::PushFont(gContext->font);
	dl->AddText(ImVec2(p.x + 3.f, p.y + 3.f), color, name ? name : "");
	ImGui::PopFont();
}

extern "C" void cvAddLine(float x0, float y0, float x1, float y1, unsigned int color)
{
	cvAssertInitialized("cvAddLine(): Canvas was not initialized");

	ImGui::GetBackgroundDrawList()->AddLine(
		ScreenFromCoords(x0, y0),
		ScreenFromCoords(x1, y1),
		color,
		gContext->config.lineThickness
	);
}

extern "C" void cvAddText(float x, float y, unsigned int color, const char* text)
{
	cvAssertInitialized("cvAddText(): Canvas was not initialized");

	ImGui::PushFont(gContext->font);
	ImGui::GetBackgroundDrawList()->AddText(
		ScreenFromCoords(x, y),
		color,
		text ? text : ""
	);
	ImGui::PopFont();
}

extern "C" void cvAddFormattedText(float x, float y, unsigned int color, const char* fmt, ...)
{
	cvAssertInitialized("cvAddFormattedText(): Canvas was not initialized");

	char buffer[2048];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	cvAddText(x, y, color, buffer);
}

// -------------------------
// Path API
// -------------------------

extern "C" void cvPathLineTo(float x, float y)
{
	cvAssertInitialized("cvPathLineTo(): Canvas was not initialized");
	ImGui::GetBackgroundDrawList()->PathLineTo(ScreenFromCoords(x, y));
}

extern "C" void cvPathStroke(unsigned int color, int closed)
{
	cvAssertInitialized("cvPathStroke(): Canvas was not initialized");
		ImGui::GetBackgroundDrawList()->PathStroke(color, closed, gContext->config.lineThickness);
}

extern "C" void cvPathFill(unsigned int color)
{
	cvAssertInitialized("cvPathFill(): Canvas was not initialized");
	ImGui::GetBackgroundDrawList()->PathFillConvex(color);
}

// -------------------------
// Texture API
// -------------------------

extern "C" CvTexture cvLoadTexture(const char* path)
{
	cvAssertInitialized("cvLoadTexture(): Canvas was not initialized");

	CvTexture tex{};

	if (!path)
		return tex;

	unsigned char* pixels = stbi_load(
		path,
		&tex.width,
		&tex.height,
		nullptr,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		fprintf(stderr, "Failed to load texture: '%s'\n", path);
		return tex;
	}

	GLuint glTex = 0;
	glGenTextures(1, &glTex);
	glBindTexture(GL_TEXTURE_2D, glTex);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		tex.width,
		tex.height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		pixels
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);

	tex.id = (CvTextureID)(uintptr_t)glTex;
	return tex;
}

extern "C" void cvUnloadTexture(CvTexture texture)
{
	cvAssertInitialized("cvUnloadTexture(): Canvas was not initialized");

	GLuint id = (GLuint)(uintptr_t)texture.id;
	if (id != 0)
		glDeleteTextures(1, &id);
}

extern "C" void cvAddTexture(float x, float y, CvTexture texture)
{
	cvAssertInitialized("cvAddTexture(): Canvas was not initialized");

	if (!texture.id)
		return;

	ImVec2 center = ScreenFromCoords(x, y);

	const float halfW = texture.width * 0.5f;
	const float halfH = texture.height * 0.5f;

	ImGui::GetBackgroundDrawList()->AddImage(
		(ImTextureID)(uintptr_t)texture.id,
		ImVec2(center.x - halfW, center.y - halfH),
		ImVec2(center.x + halfW, center.y + halfH),
		ImVec2(0, 0),
		ImVec2(1, 1),
		CV_COL32_WHITE
	);
}