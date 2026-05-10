#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void* CvTextureID;

#define CV_COL32_R_SHIFT    0
#define CV_COL32_G_SHIFT    8
#define CV_COL32_B_SHIFT    16
#define CV_COL32_A_SHIFT    24
#define CV_COL32_A_MASK     0xFF000000
#define CV_COL32(R,G,B,A)   (((A)<<CV_COL32_A_SHIFT) | ((B)<<CV_COL32_B_SHIFT) | ((G)<<CV_COL32_G_SHIFT) | ((R)<<CV_COL32_R_SHIFT))
#define CV_COL32_WHITE      CV_COL32(255,255,255,255)  // Opaque white = 0xFFFFFFFF
#define CV_COL32_BLACK      CV_COL32(0,0,0,255)        // Opaque black

typedef struct GLFWwindow GLFWwindow;

typedef struct CanvasConfig
{
	unsigned int backgroundColor;
	float fontSize; // Font size cannot be changed after cvInit
	float pointRadius;
	float lineThickness;
	int hideInternalWindow;
} CanvasConfig;

typedef struct CvTexture
{
	CvTextureID id;
	int width;
	int height;
} CvTexture;

CanvasConfig cvGetDefaultConfig(void);

// Lifecycle related functions
void cvInit(GLFWwindow* window, CanvasConfig config);
void cvShutdown(void);
void cvNewFrame(void);
void cvEndFrame(void);

// Direct access to config pointer
CanvasConfig* cvGetConfig(void);

// Coordinates related functions
void cvSetCoordinateSystemFromScreenSpace(float originX, float originY, float rightX, float rightY, float topX, float topY);
void cvCoordsToScreenSpace(float coordX, float coordY, float* screenX, float* screenY);
void cvCoordsFromScreenSpace(float screenX, float screenY, float* coordX, float* coordY);

// Draw functions
void cvAddPoint(float x, float y, unsigned int color);
void cvAddNamedPoint(float x, float y, unsigned int color, const char* name);
void cvAddLine(float x0, float y0, float x1, float y1, unsigned int color);
void cvAddText(float x0, float y0, unsigned int color, const char* text);
void cvAddFormattedText(float x0, float y0, unsigned int color, const char* format, ...);
void cvAddTexture(float x, float y, CvTexture texture);

void cvPathLineTo(float x, float y);
void cvPathStroke(unsigned int color, int closed);
void cvPathFill(unsigned int color);

// Texture functions
CvTexture cvLoadTexture(const char* path);
void cvUnloadTexture(CvTexture texture);

#ifdef __cplusplus
}
#endif
