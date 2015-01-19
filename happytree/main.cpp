/*  HappyTree
 *  version 1.0, January 10th, 2015
 *
 *  Copyright (C) 2015 Jari Komppa
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 *
 * Jari Komppa http://iki.fi/sol/
 *
 *************************************
 * 
 */

#include <Windows.h> // file i/o dialogs
#include "toolkit.h"
#include "SDL_syswm.h"
#include "../proctree/proctree.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

#define ONCE(x) { static int __once = 1; if (__once) {x;} __once = 0; }
#define CLAMP(var,min,max) { if (var < (min)) var = (min); if (var > (max)) var = (max); }


void UpdateImGui();
void InitImGui();
void ImImpl_InitGL();

//#define DEBUG_SHADOWMAPS

#define TITLE "HappyTree 20150119"

#define MAXTEXTURES 64

char *gTwigTextureName[MAXTEXTURES];
int gTwigTexture[MAXTEXTURES];
int gTwigTextureCount = 0;
char *gTrunkTextureName[MAXTEXTURES];
int gTrunkTexture[MAXTEXTURES];
int gTrunkTextureCount = 0;
int gTwigTextureIndex = -1;
int gTrunkTextureIndex = -1;
int tex_twig, tex_trunk, tex_floor;
int tex_preset[8];

int gKeyState = 0;
int gLastTick = 0;
Proctree::Tree gTree;

Proctree::Properties gOldProp(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

void init_gl_resources();

// How big a twick difference is considered 'time warp', i.e. skip the time
// (to avoid physics blowing up)
#define TICK_TIMEWARP 1000
// Physics ticks per second
#define PHYSICS_FPS 100

#define USE_PERFCOUNTERS

int inEditor()
{
	float w = gScreenWidth / 3;
	if (w < 400) w = 400;
	if (gUIState.mousex < w && ImGui::IsMouseHoveringAnyWindow())
		return 1;

	w = gScreenWidth / 5;
	if (w < 240) w = 240;
	w = gScreenWidth - (gScreenWidth / 5);
	if (gUIState.mousex > w && ImGui::IsMouseHoveringAnyWindow())
		return 1;
	return 0;
}

void export_obj(char *aFilename)
{
	FILE *f;
	f = fopen(aFilename, "wb");
	if (!f) return;
	fprintf(f, "mtllib tree.mtl\n");
	int i;
	for (i = 0; i < gTree.mVertCount; i++)
	{
		fprintf(f, "v %.16g %.16g %.16g\n", gTree.mVert[i].x, gTree.mVert[i].y, gTree.mVert[i].z);
	}
	for (i = 0; i < gTree.mTwigVertCount; i++)
	{
		fprintf(f, "v %.16g %.16g %.16g\n", gTree.mTwigVert[i].x, gTree.mTwigVert[i].y, gTree.mTwigVert[i].z);
	}
	for (i = 0; i < gTree.mVertCount; i++)
	{
		fprintf(f, "vn %.16g %.16g %.16g\n", gTree.mNormal[i].x, gTree.mNormal[i].y, gTree.mNormal[i].z);
	}
	for (i = 0; i < gTree.mTwigVertCount; i++)
	{
		fprintf(f, "vn %.16g %.16g %.16g\n", gTree.mTwigNormal[i].x, gTree.mTwigNormal[i].y, gTree.mTwigNormal[i].z);
	}
	for (i = 0; i < gTree.mVertCount; i++)
	{
		fprintf(f, "vt %.16g %.16g\n", gTree.mUV[i].u, gTree.mUV[i].v);
	}
	for (i = 0; i < gTree.mTwigVertCount; i++)
	{
		fprintf(f, "vt %.16g %.16g\n", gTree.mTwigUV[i].u, gTree.mTwigUV[i].v);
	}
	fprintf(f, "g tree\nusemtl tree\n");
	for (i = 0; i < gTree.mFaceCount; i++)
	{
		int a = gTree.mFace[i].x + 1;
		int b = gTree.mFace[i].y + 1;
		int c = gTree.mFace[i].z + 1;
		fprintf(f, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", a, a, a, b, b, b, c, c, c);
	}
	fprintf(f, "g twig\nusemtl twig\n");
	for (i = 0; i < gTree.mTwigFaceCount; i++)
	{
		int a = gTree.mTwigFace[i].x + gTree.mVertCount + 1;
		int b = gTree.mTwigFace[i].y + gTree.mVertCount + 1;
		int c = gTree.mTwigFace[i].z + gTree.mVertCount + 1;
		fprintf(f, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", a, a, a, b, b, b, c, c, c);
	}
	fclose(f);
}

void export_h(char *aFilename)
{
	FILE * f = fopen(aFilename, "w");
	if (!f) return;

	fprintf(f, "Properties prop(\n"
		"%ff, // float aClumpMax,\n"
		"%ff, // float aClumpMin,\n"
		"%ff, // float aLengthFalloffFactor,\n"
		"%ff, // float aLengthFalloffPower,\n"
		"%ff, // float aBranchFactor,\n"
		"%ff, // float aRadiusFalloffRate,\n"
		"%ff, // float aClimbRate,\n"
		"%ff, // float aTrunkKink,\n"
		"%ff, // float aMaxRadius,\n"
		"%d, // int aTreeSteps,\n"
		"%ff, // float aTaperRate,\n"
		"%ff, // float aTwistRate,\n"
		"%d, // int aSegments,\n"
		"%d, // int aLevels,\n"
		"%ff, // float aSweepAmount,\n"
		"%ff, // float aInitialBranchLength,\n"
		"%ff, // float aTrunkLength,\n"
		"%ff, // float aDropAmount,\n"
		"%ff, // float aGrowAmount,\n"
		"%ff, // float aVMultiplier,\n"
		"%ff, // float aTwigScale,\n"
		"%d); // int aSeed\n",
		gTree.mProperties.mClumpMax,
		gTree.mProperties.mClumpMin,
		gTree.mProperties.mLengthFalloffFactor,
		gTree.mProperties.mLengthFalloffPower,
		gTree.mProperties.mBranchFactor,
		gTree.mProperties.mRadiusFalloffRate,
		gTree.mProperties.mClimbRate,
		gTree.mProperties.mTrunkKink,
		gTree.mProperties.mMaxRadius,
		gTree.mProperties.mTreeSteps,
		gTree.mProperties.mTaperRate,
		gTree.mProperties.mTwistRate,
		gTree.mProperties.mSegments,
		gTree.mProperties.mLevels,
		gTree.mProperties.mSweepAmount,
		gTree.mProperties.mInitialBranchLength,
		gTree.mProperties.mTrunkLength,
		gTree.mProperties.mDropAmount,
		gTree.mProperties.mGrowAmount,
		gTree.mProperties.mVMultiplier,
		gTree.mProperties.mTwigScale,
		gTree.mProperties.mSeed);
	fclose(f);
}

void load_htr(char *aFilename)
{
	FILE *f = fopen(aFilename, "rb");
	if (!f) return;
	int sig;
	fread(&sig, 1, sizeof(int), f);
	if (sig != 0x00525448)
	{
		fclose(f);
		MessageBoxA(NULL, "Error loading file: signature not recognized.", "Error loading file", MB_ICONERROR);
		return;
	}
	fread(&gTree.mProperties.mClumpMax, 1, sizeof(float), f);
	fread(&gTree.mProperties.mClumpMin, 1, sizeof(float), f);
	fread(&gTree.mProperties.mLengthFalloffFactor, 1, sizeof(float), f);
	fread(&gTree.mProperties.mLengthFalloffPower, 1, sizeof(float), f);
	fread(&gTree.mProperties.mBranchFactor, 1, sizeof(float), f);
	fread(&gTree.mProperties.mRadiusFalloffRate, 1, sizeof(float), f);
	fread(&gTree.mProperties.mClimbRate, 1, sizeof(float), f);
	fread(&gTree.mProperties.mTrunkKink, 1, sizeof(float), f);
	fread(&gTree.mProperties.mMaxRadius, 1, sizeof(float), f);
	fread(&gTree.mProperties.mTreeSteps, 1, sizeof(int), f);
	fread(&gTree.mProperties.mTaperRate, 1, sizeof(float), f);
	fread(&gTree.mProperties.mTwistRate, 1, sizeof(float), f);
	fread(&gTree.mProperties.mSegments, 1, sizeof(int), f);
	fread(&gTree.mProperties.mLevels, 1, sizeof(int), f);
	fread(&gTree.mProperties.mSweepAmount, 1, sizeof(float), f);
	fread(&gTree.mProperties.mInitialBranchLength, 1, sizeof(float), f);
	fread(&gTree.mProperties.mTrunkLength, 1, sizeof(float), f);
	fread(&gTree.mProperties.mDropAmount, 1, sizeof(float), f);
	fread(&gTree.mProperties.mGrowAmount, 1, sizeof(float), f);
	fread(&gTree.mProperties.mVMultiplier, 1, sizeof(float), f);
	fread(&gTree.mProperties.mTwigScale, 1, sizeof(float), f);
	fread(&gTree.mProperties.mSeed, 1, sizeof(int), f);
	fclose(f);
}

void save_htr(char *aFilename)
{
	FILE *f = fopen(aFilename, "wb");
	if (!f) return;
	int sig = 0x00525448;
	fwrite(&sig, 1, sizeof(int), f);
	fwrite(&gTree.mProperties.mClumpMax, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mClumpMin, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mLengthFalloffFactor, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mLengthFalloffPower, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mBranchFactor, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mRadiusFalloffRate, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mClimbRate, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mTrunkKink, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mMaxRadius, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mTreeSteps, 1, sizeof(int), f);
	fwrite(&gTree.mProperties.mTaperRate, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mTwistRate, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mSegments, 1, sizeof(int), f);
	fwrite(&gTree.mProperties.mLevels, 1, sizeof(int), f);
	fwrite(&gTree.mProperties.mSweepAmount, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mInitialBranchLength, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mTrunkLength, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mDropAmount, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mGrowAmount, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mVMultiplier, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mTwigScale, 1, sizeof(float), f);
	fwrite(&gTree.mProperties.mSeed, 1, sizeof(int), f);
	fclose(f);
}

void progress()
{
	static int lastprogresstick = 0;
	int tick = SDL_GetTicks();
	if (tick - lastprogresstick < (1000 / 20))
		return;
	lastprogresstick = tick;
	float f = tick / 1000.0f;
	glClearColor(0, 0, 0, 1);
	glColor3f(1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glVertex2f(sin(f) * 0.1 + 0, cos(f) * 0.1 + 0); f += 3.14 * 4 / 3;
	glVertex2f(sin(f) * 0.1 + 0, cos(f) * 0.1 + 0); f += 3.14 * 4 / 3;
	glVertex2f(sin(f) * 0.1 + 0, cos(f) * 0.11 + 0);
	glEnd();
	SDL_GL_SwapBuffers();
	
}


char * loadfile(char *aFilename, int &aLen)
{
	// There's some bit of code that every programmer finds themselves rewriting over
	// and over and over and OVER again. For myself, it's this. I've written this function
	// innumerable times. Why can't "just give me the data" be in standard libraries?
	FILE * f = fopen(aFilename, "rb");
	if (!f)
	{
		aLen = 0;
		return 0;
	}
	fseek(f, 0, SEEK_END);
	aLen = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = new char[aLen + 1];
	buf[aLen] = 0;
	fread(buf, 1, aLen, f);
	fclose(f);
	return buf;
}

class Shader
{
public:
	char *mVSSrc;
	int mVSLen;
	char *mFSSrc;
	int mFSLen;
	GLuint mShaderHandle;

	Shader()
	{
		mVSSrc = 0;
		mVSLen = 0;
		mFSSrc = 0;
		mFSLen = 0;
	}

	~Shader()
	{
		delete[] mFSSrc;
		delete[] mVSSrc;
	}

	void init(char *aFilename_vs, char *aFilename_fs)
	{
		delete[] mVSSrc;
		delete[] mFSSrc;
		mVSSrc = loadfile(aFilename_vs, mVSLen);
		mFSSrc = loadfile(aFilename_fs, mFSLen);
	}

	void build()
	{
		char *vs_src_p[1] = { mVSSrc };
		char *fs_src_p[1] = { mFSSrc };

		int vs_obj = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs_obj, 1, (const char**)vs_src_p, &mVSLen);
		glCompileShader(vs_obj);

		int fs_obj = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs_obj, 1, (const char**)fs_src_p, &mFSLen);
		glCompileShader(fs_obj);

		mShaderHandle = glCreateProgram();
		glAttachShader(mShaderHandle, vs_obj);
		glAttachShader(mShaderHandle, fs_obj);
		glLinkProgram(mShaderHandle);
		int status;
		glGetProgramiv(mShaderHandle, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			char temp[2048];
			glGetProgramInfoLog(mShaderHandle, 2048, &status, temp);
			MessageBoxA(NULL, temp, "Shader link failure", MB_ICONERROR);
		}
	}

	void use()
	{
		glUseProgram(mShaderHandle);	
	}

	int uniformLocation(char *aName)
	{
		return glGetUniformLocation(mShaderHandle, aName);
	}
};
Shader gBaseShader, gShadowpassShader;
#ifdef DEBUG_SHADOWMAPS
Shader gShadowDebugShader;
#endif


int propertiesChanged()
{
	int ret = 0;
	if (gTree.mProperties.mClumpMax != gOldProp.mClumpMax ||
		gTree.mProperties.mClumpMin != gOldProp.mClumpMin ||
		gTree.mProperties.mLengthFalloffFactor != gOldProp.mLengthFalloffFactor ||
		gTree.mProperties.mLengthFalloffPower != gOldProp.mLengthFalloffPower ||
		gTree.mProperties.mBranchFactor != gOldProp.mBranchFactor ||
		gTree.mProperties.mRadiusFalloffRate != gOldProp.mRadiusFalloffRate ||
		gTree.mProperties.mClimbRate != gOldProp.mClimbRate ||
		gTree.mProperties.mTrunkKink != gOldProp.mTrunkKink ||
		gTree.mProperties.mMaxRadius != gOldProp.mMaxRadius ||
		gTree.mProperties.mTreeSteps != gOldProp.mTreeSteps ||
		gTree.mProperties.mTaperRate != gOldProp.mTaperRate ||
		gTree.mProperties.mTwistRate != gOldProp.mTwistRate ||
		gTree.mProperties.mSegments != gOldProp.mSegments ||
		gTree.mProperties.mLevels != gOldProp.mLevels ||
		gTree.mProperties.mSweepAmount != gOldProp.mSweepAmount ||
		gTree.mProperties.mInitialBranchLength != gOldProp.mInitialBranchLength ||
		gTree.mProperties.mTrunkLength != gOldProp.mTrunkLength ||
		gTree.mProperties.mDropAmount != gOldProp.mDropAmount ||
		gTree.mProperties.mGrowAmount != gOldProp.mGrowAmount ||
		gTree.mProperties.mVMultiplier != gOldProp.mVMultiplier ||
		gTree.mProperties.mTwigScale != gOldProp.mTwigScale ||
		gTree.mProperties.mSeed != gOldProp.mSeed)
	{
		ret = 1;
		gOldProp = gTree.mProperties;
	}
	return ret;
}


void handle_key(int keysym, int down)
{
    switch(keysym)
    {
    case SDLK_ESCAPE:
        if (!down)
        {
            exit(0);
        }
        break;
    case SDLK_UP:
    case SDLK_w:
        if (down) gKeyState |= KEY_UP; else gKeyState &= ~KEY_UP;
        break;
    case SDLK_DOWN:
    case SDLK_s:
        if (down) gKeyState |= KEY_DOWN; else gKeyState &= ~KEY_DOWN;
        break;
    case SDLK_LEFT:
    case SDLK_a:
        if (down) gKeyState |= KEY_LEFT; else gKeyState &= ~KEY_LEFT;
        break;
    case SDLK_RIGHT:
    case SDLK_d:
        if (down) gKeyState |= KEY_RIGHT; else gKeyState &= ~KEY_RIGHT;
        break;
    case SDLK_RCTRL:
    case SDLK_LCTRL:
        if (down) gKeyState |= KEY_FIRE;
        break;
    }
}

void process_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) 
    {	
		switch (event.type)
		{
		case SDL_KEYDOWN:
			handle_key(event.key.keysym.sym, 1);
			// If a key is pressed, report it to the widgets
			gUIState.keyentered = event.key.keysym.sym;
			gUIState.keymod = event.key.keysym.mod;
			// if key is ASCII, accept it as character input
			if ((event.key.keysym.unicode & 0xFF80) == 0)
				gUIState.keychar = event.key.keysym.unicode & 0x7f;
			break;
		case SDL_KEYUP:
			handle_key(event.key.keysym.sym, 0);
			break;
		case SDL_MOUSEMOTION:
			// update mouse position
			gUIState.mousex = event.motion.x;
			gUIState.mousey = event.motion.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			// update button down state if left-clicking
			if (event.button.button == 1)
			{
				gUIState.mousedown = 1;
				gUIState.mousedownx = event.motion.x;
				gUIState.mousedowny = event.motion.y;
				if (!inEditor())
					gUIState.not_imgui_mousedown = 1;
			}
			if (event.button.button == 4)
			{
				gUIState.scroll = +1;
			}
			if (event.button.button == 5)
			{
				gUIState.scroll = -1;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			// update button down state if left-clicking
			if (event.button.button == 1)
			{
				gUIState.mousedown = 0;
				gUIState.not_imgui_mousedown = 0;
			}
			break;
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		case SDL_VIDEORESIZE:
			gScreenWidth = event.resize.w;
			gScreenHeight = event.resize.h;				
			initvideo(0);
			init_gl_resources();
			break;
		}
    }
}



void physics_tick(int tick)
{

}

int gForestMode = 0;
int gTextureMode = 1;
int gTwigMode = 1;
int gWireframeMode = 0;
int gLightingMode = 1;
int gShadowMode = 1;
int gAnimateLight = 0;
float gSkyColor[3] = { 70 / 256.0f, 112 / 256.0f, 175 / 256.0f };
glm::vec3 gLightDir = { 1, 1, 0 };

GLuint gVertVBO = 0;
GLuint gNormalVBO = 0;
GLuint gUVVBO = 0;
GLuint gFaceVBO = 0;
GLuint gTwigVertVBO = 0;
GLuint gTwigNormalVBO = 0;
GLuint gTwigUVVBO = 0;
GLuint gTwigFaceVBO = 0;

GLuint gFloorVertVBO = 0;
GLuint gFloorNormalVBO = 0;
GLuint gFloorUVVBO = 0;

GLuint rb_shadowfbo, rb_shadow;
#define RB_WIDTH 1024
#define RB_HEIGHT 1024


void init_gl_resources()
{
	ImImpl_InitGL();

	gOldProp.mSeed = -1; // force regen
	glGenBuffers(1, &gVertVBO);
	glGenBuffers(1, &gNormalVBO);
	glGenBuffers(1, &gUVVBO);
	glGenBuffers(1, &gFaceVBO);
	glGenBuffers(1, &gTwigVertVBO);
	glGenBuffers(1, &gTwigNormalVBO);
	glGenBuffers(1, &gTwigUVVBO);
	glGenBuffers(1, &gTwigFaceVBO);

	glGenBuffers(1, &gFloorVertVBO);
	glGenBuffers(1, &gFloorNormalVBO);
	glGenBuffers(1, &gFloorUVVBO);

	float floorvert[256 * 3];
	float floornormal[256 * 3];
	float flooruv[256 * 2];
	int i;
	for (i = 0; i < 256; i++)
	{
		floorvert[i * 3 + 0] = sin(i / 128.0 * M_PI) * 100;
		floorvert[i * 3 + 1] = 0;
		floorvert[i * 3 + 2] = cos(i / 128.0 * M_PI) * 100;
		floornormal[i * 3 + 0] = 0;
		floornormal[i * 3 + 1] = -1;
		floornormal[i * 3 + 2] = 0;
		flooruv[i * 2 + 0] = ((sin(i / 128.0 * M_PI) + 1) / 2) * 20;
		flooruv[i * 2 + 1] = ((cos(i / 128.0 * M_PI) + 1) / 2) * 20;
	}

	glBindBuffer(GL_ARRAY_BUFFER, gFloorVertVBO);
	glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(float) * 3, floorvert, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, gFloorNormalVBO);
	glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(float) * 3, floornormal, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, gFloorUVVBO);
	glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(float) * 2, flooruv, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// create a framebuffer object for shadows
	glGenFramebuffersEXT(1, &rb_shadowfbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER, rb_shadowfbo);

	if (!rb_shadow)
		glGenTextures(1, &rb_shadow);
	glBindTexture(GL_TEXTURE_2D, rb_shadow);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, RB_WIDTH, RB_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);	
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
	*/
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rb_shadow, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);

	gBaseShader.build();
	gShadowpassShader.build();
#ifdef DEBUG_SHADOWMAPS
	gShadowDebugShader.build();
#endif
}

glm::mat4 mat_proj;
glm::mat4 mat_modelview;
glm::mat4 mat_shadow;

glm::vec3 gCamRotate = { 0, 0.2, 20 };

void calc_shadowmatrix()
{
	glm::mat4 bias = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0 };
	mat_shadow = bias;
	mat_shadow *= mat_proj;
	mat_shadow *= mat_modelview;
}

void setup_shadow()
{
	gShadowpassShader.use();
	GLuint texturepos = gShadowpassShader.uniformLocation("tex");
	
	glUniform1i(texturepos, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, rb_shadowfbo);

	glViewport(0, 0, RB_WIDTH, RB_HEIGHT);

	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	float d = gCamRotate.z + gForestMode * 20;

	mat_proj = glm::ortho<float>(-d, d, -d, d, 10, 100);

	mat_modelview = glm::mat4();
	int tick = 0;
	mat_modelview *= glm::lookAt(
		gLightDir * 20.0f,
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0));

	calc_shadowmatrix();
}

void setup_rendering(int tick)
{
	glViewport(0, 0, gScreenWidth, gScreenHeight);

	
	int lt = tick;
	if (!gAnimateLight)
		lt = 0;

	gLightDir.x = sin(lt * 0.00023);
	gLightDir.z = cos(lt * 0.00023);
	gLightDir.y = 1;
	gLightDir = glm::normalize(gLightDir);


	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gBaseShader.use();
	GLuint lightingpos = gBaseShader.uniformLocation("EnableLighting");
	GLuint texturingpos = gBaseShader.uniformLocation("EnableTexture");
	GLuint shadowingpos = gBaseShader.uniformLocation("EnableShadows");
	GLuint shadowmatrixpos = gBaseShader.uniformLocation("ShadowMatrix");
	GLuint texturepos = gBaseShader.uniformLocation("tex");
	GLuint shadowmappos = gBaseShader.uniformLocation("shadowmap");
	GLuint lightdir = gBaseShader.uniformLocation("lightdir");
	glUniform1i(lightingpos, gLightingMode);
	glUniform1i(texturingpos, gTextureMode);
	glUniform1i(shadowingpos, gShadowMode);
	glUniform1i(texturepos, 0);
	glUniform1i(shadowmappos, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rb_shadow);
	glActiveTexture(GL_TEXTURE0);
	glUniformMatrix4fv(shadowmatrixpos, 1, GL_FALSE, &mat_shadow[0][0]);
	glUniform3fv(lightdir, 1, &gLightDir[0]);

	mat_proj = glm::perspective((float)M_PI * 60.0f / 360.0f, gScreenWidth / (float)gScreenHeight, 1.0f, 1000.0f);

	mat_modelview = glm::mat4();

	float d = gCamRotate.z + gForestMode * 20;
	glm::vec3 at = { cos(gCamRotate.x), sin(gCamRotate.y), sin(gCamRotate.x) };
	at = glm::normalize(at) * d;
	mat_modelview *= glm::lookAt(
		at,//glm::vec3(cos(tick * 0.0002f) * d, 4, sin(tick * 0.0002f) * d),
		glm::vec3(0, 4, 0),
		glm::vec3(0, 1, 0));

}


void draw_floor(Shader &shader)
{
	GLuint matrixpos = shader.uniformLocation("RotationMatrix");

	glm::mat4 mat;

	mat = mat_proj * mat_modelview;

	glUniformMatrix4fv(matrixpos, 1, GL_FALSE, &mat[0][0]);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glBindTexture(GL_TEXTURE_2D, tex_floor);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, gFloorVertVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gFloorNormalVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gFloorUVVBO);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 256);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

}

void prep_draw_tree()
{
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindTexture(GL_TEXTURE_2D, tex_trunk);

	glBindBuffer(GL_ARRAY_BUFFER, gVertVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gNormalVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gUVVBO);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gFaceVBO);
}

void finish_draw_tree()
{
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

void draw_tree(Shader &shader)
{
	GLuint matrixpos = shader.uniformLocation("RotationMatrix");

	glm::mat4 mat;

	mat = mat_proj * mat_modelview;

	glUniformMatrix4fv(matrixpos, 1, GL_FALSE, &mat[0][0]);

	glDrawElements(GL_TRIANGLES, gTree.mFaceCount * 3, GL_UNSIGNED_INT, 0);

}

void prep_draw_twig()
{
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, tex_twig);

	//glDepthMask(GL_FALSE);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, gTwigVertVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gTwigNormalVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gTwigUVVBO);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gTwigFaceVBO);
}


void draw_twig(Shader &shader)
{
	GLuint matrixpos = shader.uniformLocation("RotationMatrix");

	glm::mat4 mat;

	mat = mat_proj * mat_modelview;

	glUniformMatrix4fv(matrixpos, 1, GL_FALSE, &mat[0][0]);

	glDrawElements(GL_TRIANGLES, gTree.mTwigFaceCount * 3, GL_UNSIGNED_INT, 0);
}


void finish_draw_twig()
{
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glEnable(GL_CULL_FACE);

}

#ifdef DEBUG_SHADOWMAPS
void shadowmap_debug()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gShadowDebugShader.use();
	GLuint matrixpos = gShadowDebugShader.uniformLocation("RotationMatrix");
	GLuint texpos = gShadowDebugShader.uniformLocation("tex");
	glUniform1i(texpos, 0);
	glm::mat4 mat;
	glUniformMatrix4fv(matrixpos, 1, GL_FALSE, &mat[0][0]);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rb_shadow);
	
	float coords[] =
	{
		0, 0, 0,
		1, 0, 0,
		0, 1, 0,
		1, 1, 0
	};
	
	float uvcoords[] =
	{
		0, 0, 
		1, 0, 
		0, 1, 
		1, 1, 
	};
	GLuint vbo, vbouv;

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &vbouv);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float) * 3, coords, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbouv);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float) * 2, uvcoords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, vbouv);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbouv);
}
#endif

void draw_imgui()
{
	UpdateImGui();

	ImVec2 previewsize(200, 200);

	bool yah = true;
	ImGui::GetStyle().FramePadding = { 0, 0 };
	ImGui::GetStyle().WindowPadding = { 8, 4 };
	ImGui::GetStyle().ItemSpacing = { 0, 1 };
	ImGui::GetStyle().TreeNodeSpacing = 0;
	

	ImGui::SetNextWindowPos({ 0, 0 });
	float w = gScreenWidth / 3;
	if (w < 400) w = 400;
	ImGui::SetNextWindowSize({ w, (float)gScreenHeight });
	ImGui::Begin("Tree", &yah, ImVec2(0, 0), 0.2f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	ONCE(ImGui::OpenNextNode(1));
	if (ImGui::TreeNode("Geometry stats"))
	{
		ImGui::Text("Trunk verts %8d", gTree.mVertCount);
		ImGui::Text("Trunk faces %8d", gTree.mFaceCount);
		ImGui::Text("Twig verts  %8d", gTree.mTwigVertCount);
		ImGui::Text("Twig faces  %8d", gTree.mTwigFaceCount);
	}

	ONCE(ImGui::OpenNextNode(1));
	if (ImGui::TreeNode("General"))
	{                  
		ImGui::InputInt("Random seed", &gTree.mProperties.mSeed);
		ImGui::InputInt("Branch segments", &gTree.mProperties.mSegments, 2);
		CLAMP(gTree.mProperties.mSegments, 4, 32);	
		ImGui::InputInt("Branch levels", &gTree.mProperties.mLevels);
		CLAMP(gTree.mProperties.mLevels, 3, 32);
		ImGui::InputInt("Trunk forks", &gTree.mProperties.mTreeSteps);
		CLAMP(gTree.mProperties.mTreeSteps, 0, 32);
		ImGui::SliderFloat("Texture V multiplier", &gTree.mProperties.mVMultiplier, 0.01f, 10.0f, "%.3f", 4.0f);
		ImGui::SliderFloat("Twig scale", &gTree.mProperties.mTwigScale, 0.01f, 2.0f);
	}
	ONCE(ImGui::OpenNextNode(1));
	if (ImGui::TreeNode("Branching"))
	{		
		ImGui::SliderFloat("Initial length", &gTree.mProperties.mInitialBranchLength,  0.01f, 5.0f);
		ImGui::SliderFloat("Len falloff rate", &gTree.mProperties.mLengthFalloffFactor, 0.01f, 1.5f);
		ImGui::SliderFloat("Len falloff power", &gTree.mProperties.mLengthFalloffPower, -2.0f, 2.0f);
		ImGui::SliderFloat("Max clumping", &gTree.mProperties.mClumpMax, 0.01f, 10.0f);
		ImGui::SliderFloat("Min clumping", &gTree.mProperties.mClumpMin, 0.01f, 10.0f);
		ImGui::SliderFloat("Symmetry", &gTree.mProperties.mBranchFactor, 2.0f, 4.0f);
		ImGui::SliderFloat("Droop", &gTree.mProperties.mDropAmount, -2.0f, 2.0f);
		ImGui::SliderFloat("Growth", &gTree.mProperties.mGrowAmount, -4.0f, 4.0f);
		ImGui::SliderFloat("Sweep", &gTree.mProperties.mSweepAmount, -1.0f, 1.0f);
	}
	ONCE(ImGui::OpenNextNode(1));
	if (ImGui::TreeNode("Trunk"))
	{
		ImGui::SliderFloat("Trunk radius", &gTree.mProperties.mMaxRadius, 0.01f, 0.5f);
		ImGui::SliderFloat("Radius falloff", &gTree.mProperties.mRadiusFalloffRate, 0.1f, 1.0f);
		ImGui::SliderFloat("Climb rate", &gTree.mProperties.mClimbRate, 0.01f, 1.0f);
		ImGui::SliderFloat("Kink", &gTree.mProperties.mTrunkKink, -2.0f, 2.0f);
		ImGui::SliderFloat("Taper rate", &gTree.mProperties.mTaperRate, 0.5f, 2.0f);
		ImGui::SliderFloat("Twists", &gTree.mProperties.mTwistRate, 0.01f, 10.0f, "%.3f",4.0f);
		ImGui::SliderFloat("Trunk length", &gTree.mProperties.mTrunkLength, 0.01f, 5.0f);
	}
	ONCE(ImGui::OpenNextNode(1));
	if (ImGui::TreeNode("Project"))
	{
		if (ImGui::Button("Save project"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "HappyTree project .htr\0*.HTR\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "HTR";
			ofn.lpstrTitle = "HappyTree project .HTR";

			if (GetSaveFileNameA(&ofn))
			{
				save_htr(temp);
			}
		}
		if (ImGui::Button("Load project"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "HappyTree project .HTR\0*.HTR\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "htr";
			ofn.lpstrTitle = "HappyTree project .HTR";

			if (GetOpenFileNameA(&ofn))
			{
				load_htr(temp);
			}
		}
	}
	if (ImGui::TreeNode("Custom textures"))
	{
		if (ImGui::Button("Load custom trunk texture"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "All image files\0*.JPG;*.PNG;*.TGA;*.bmp;*.PSD;*.GIF;*.HDR;*.PIC\0JPEG\0*.JPG\0PNG\0*.PNG\0TGA\0*.TGA\0BMP\0*.bmp\0PSD\0*.PSD\0GIF\0*.GIF\0HDR\0*.HDR\0PIC\0*.PIC\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "png";
			ofn.lpstrTitle = "Texture file";

			if (GetOpenFileNameA(&ofn))
			{
				tex_trunk = load_texture(temp, 0);
				gTrunkTextureIndex = -1;
			}
		}
		if (ImGui::Button("Load custom twig texture"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "All image files\0*.JPG;*.PNG;*.TGA;*.bmp;*.PSD;*.GIF;*.HDR;*.PIC\0JPEG\0*.JPG\0PNG\0*.PNG\0TGA\0*.TGA\0BMP\0*.bmp\0PSD\0*.PSD\0GIF\0*.GIF\0HDR\0*.HDR\0PIC\0*.PIC\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "png";
			ofn.lpstrTitle = "Texture file";

			if (GetOpenFileNameA(&ofn))
			{
				tex_twig = load_texture(temp, 0);
				gTwigTextureIndex = -1;
			}
		}
		if (ImGui::Button("Load custom floor texture"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "All image files\0*.JPG;*.PNG;*.TGA;*.bmp;*.PSD;*.GIF;*.HDR;*.PIC\0JPEG\0*.JPG\0PNG\0*.PNG\0TGA\0*.TGA\0BMP\0*.bmp\0PSD\0*.PSD\0GIF\0*.GIF\0HDR\0*.HDR\0PIC\0*.PIC\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "png";
			ofn.lpstrTitle = "Texture file";

			if (GetOpenFileNameA(&ofn))
			{
				tex_floor = load_texture(temp, 0);
			}
		}
	}
	if (ImGui::TreeNode("Export"))
	{
		if (ImGui::Button("Export mesh as .obj"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "LightWave .obj\0*.obj\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "obj";
			ofn.lpstrTitle = "LightWave .obj";

			if (GetSaveFileNameA(&ofn))
			{
				export_obj(temp);
			}
		}
		if (ImGui::Button("Export params as c++ .h"))
		{
			SDL_SysWMinfo sysinfo;
			SDL_VERSION(&sysinfo.version);
			SDL_GetWMInfo(&sysinfo);
			OPENFILENAMEA ofn;
			char szFileName[1024] = "";

			ZeroMemory(&ofn, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = sysinfo.window;
			ofn.lpstrFilter = "C++ .h\0*.h\0All files\0*.*\0\0";

			ofn.nMaxFile = 1024;

			ofn.Flags = OFN_EXPLORER;
			char temp[1024]; temp[0] = 0;
			ofn.lpstrFile = temp;
			ofn.lpstrDefExt = "h";
			ofn.lpstrTitle = "C++ .h";

			if (GetSaveFileNameA(&ofn))
			{
				export_h(temp);
			}
		}
	}

	if (ImGui::TreeNode("Editor features"))
	{
		ImGui::SliderInt("Forest mode", &gForestMode, 0, 1);
		ImGui::SliderInt("Enable textures", &gTextureMode, 0, 1);
		ImGui::SliderInt("Wireframe mode", &gWireframeMode, 0, 1);
		ImGui::SliderInt("Draw twigs", &gTwigMode, 0, 1);
		ImGui::SliderInt("Lighting", &gLightingMode, 0, 1);
		ImGui::SliderInt("Shadows", &gShadowMode, 0, 1);
		ImGui::SliderInt("Animate light", &gAnimateLight, 0, 1);
		ImGui::SliderFloat3("Sky color", gSkyColor, 0, 1);
	}

	ImGui::End();

	w = gScreenWidth / 5;
	if (w < 240) w = 240;
	ImGui::SetNextWindowPos({ gScreenWidth - w, 0 });
	ImGui::SetNextWindowSize({ w, (float)gScreenHeight });
	ImGui::Begin("Content browser", &yah, ImVec2(0, 0), 0.2f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);


	if (ImGui::TreeNode("Presets"))
	{		
		ImGui::Image((ImTextureID)tex_preset[0], previewsize);
		
		if (ImGui::Button("Load preset 1"))
		{
			gTree.mProperties.mSeed = 262;
			gTree.mProperties.mSegments = 6;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 0.36f;
			gTree.mProperties.mTwigScale = 0.39f;
			gTree.mProperties.mInitialBranchLength = 0.49f;
			gTree.mProperties.mLengthFalloffFactor = 0.85f;
			gTree.mProperties.mLengthFalloffPower = 0.99f;
			gTree.mProperties.mClumpMax = 0.454f;
			gTree.mProperties.mClumpMin = 0.404f;
			gTree.mProperties.mBranchFactor = 2.45f;
			gTree.mProperties.mDropAmount = -0.1f;
			gTree.mProperties.mGrowAmount = 0.235f;
			gTree.mProperties.mSweepAmount = 0.01f;
			gTree.mProperties.mMaxRadius = 0.139f;
			gTree.mProperties.mClimbRate = 0.371f;
			gTree.mProperties.mTrunkKink = 0.093f;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.947f;
			gTree.mProperties.mRadiusFalloffRate = 0.73f;
			gTree.mProperties.mTwistRate = 3.02f;
			gTree.mProperties.mTrunkLength = 2.4f;
		}
		ImGui::Image((ImTextureID)tex_preset[1], previewsize);

		if (ImGui::Button("Load preset 2"))
		{
			gTree.mProperties.mSeed = 861;
			gTree.mProperties.mSegments = 10;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 0.66;
			gTree.mProperties.mTwigScale = 0.47;
			gTree.mProperties.mInitialBranchLength = 0.5;
			gTree.mProperties.mLengthFalloffFactor = 0.85;
			gTree.mProperties.mLengthFalloffPower = 0.99;
			gTree.mProperties.mClumpMax = 0.449;
			gTree.mProperties.mClumpMin = 0.404;
			gTree.mProperties.mBranchFactor = 2.75;
			gTree.mProperties.mDropAmount = 0.07;
			gTree.mProperties.mGrowAmount = -0.005;
			gTree.mProperties.mSweepAmount = 0.01;
			gTree.mProperties.mMaxRadius = 0.269;
			gTree.mProperties.mClimbRate = 0.626;
			gTree.mProperties.mTrunkKink = 0.108;
			gTree.mProperties.mTreeSteps = 4;
			gTree.mProperties.mTaperRate = 0.876;
			gTree.mProperties.mRadiusFalloffRate = 0.66;
			gTree.mProperties.mTwistRate = 2.7;
			gTree.mProperties.mTrunkLength = 1.55;
		}
		ImGui::Image((ImTextureID)tex_preset[2], previewsize);

		if (ImGui::Button("Load preset 3"))
		{
			gTree.mProperties.mSeed = 152;
			gTree.mProperties.mSegments = 6;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 1.16;
			gTree.mProperties.mTwigScale = 0.44;
			gTree.mProperties.mInitialBranchLength = 0.49;
			gTree.mProperties.mLengthFalloffFactor = 0.85;
			gTree.mProperties.mLengthFalloffPower = 0.99;
			gTree.mProperties.mClumpMax = 0.454;
			gTree.mProperties.mClumpMin = 0.246;
			gTree.mProperties.mBranchFactor = 3.2;
			gTree.mProperties.mDropAmount = 0.09;
			gTree.mProperties.mGrowAmount = 0.235;
			gTree.mProperties.mSweepAmount = 0.01;
			gTree.mProperties.mMaxRadius = 0.111;
			gTree.mProperties.mClimbRate = 0.41;
			gTree.mProperties.mTrunkKink = 0;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.835;
			gTree.mProperties.mRadiusFalloffRate = 0.73;
			gTree.mProperties.mTwistRate = 2.06;
			gTree.mProperties.mTrunkLength = 2.45;
		}
		ImGui::Image((ImTextureID)tex_preset[3], previewsize);

		if (ImGui::Button("Load preset 4"))
		{
			gTree.mProperties.mSeed = 499;
			gTree.mProperties.mSegments = 8;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 1;
			gTree.mProperties.mTwigScale = 0.28;
			gTree.mProperties.mInitialBranchLength = 0.5;
			gTree.mProperties.mLengthFalloffFactor = 0.98;
			gTree.mProperties.mLengthFalloffPower = 1.08;
			gTree.mProperties.mClumpMax = 0.414;
			gTree.mProperties.mClumpMin = 0.282;
			gTree.mProperties.mBranchFactor = 2.2;
			gTree.mProperties.mDropAmount = 0.24;
			gTree.mProperties.mGrowAmount = 0.044;
			gTree.mProperties.mSweepAmount = 0;
			gTree.mProperties.mMaxRadius = 0.096;
			gTree.mProperties.mClimbRate = 0.39;
			gTree.mProperties.mTrunkKink = 0;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.958;
			gTree.mProperties.mRadiusFalloffRate = 0.71;
			gTree.mProperties.mTwistRate = 2.97;
			gTree.mProperties.mTrunkLength = 1.95;
		}
		ImGui::Image((ImTextureID)tex_preset[4], previewsize);

		if (ImGui::Button("Load preset 5"))
		{
			gTree.mProperties.mSeed = 267;
			gTree.mProperties.mSegments = 8;
			gTree.mProperties.mLevels = 4;
			gTree.mProperties.mVMultiplier = 0.96;
			gTree.mProperties.mTwigScale = 0.71;
			gTree.mProperties.mInitialBranchLength = 0.12;
			gTree.mProperties.mLengthFalloffFactor = 1;
			gTree.mProperties.mLengthFalloffPower = 0.7;
			gTree.mProperties.mClumpMax = 0.556;
			gTree.mProperties.mClumpMin = 0.404;
			gTree.mProperties.mBranchFactor = 3.5;
			gTree.mProperties.mDropAmount = 0.18;
			gTree.mProperties.mGrowAmount = -0.108;
			gTree.mProperties.mSweepAmount = 0.01;
			gTree.mProperties.mMaxRadius = 0.139;
			gTree.mProperties.mClimbRate = 0.419;
			gTree.mProperties.mTrunkKink = 0.093;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.947;
			gTree.mProperties.mRadiusFalloffRate = 0.73;
			gTree.mProperties.mTwistRate = 3.53;
			gTree.mProperties.mTrunkLength = 1.75;
		}
		ImGui::Image((ImTextureID)tex_preset[5], previewsize);

		if (ImGui::Button("Load preset 6"))
		{
			gTree.mProperties.mSeed = 519;
			gTree.mProperties.mSegments = 6;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 1.01;
			gTree.mProperties.mTwigScale = 0.52;
			gTree.mProperties.mInitialBranchLength = 0.65;
			gTree.mProperties.mLengthFalloffFactor = 0.73;
			gTree.mProperties.mLengthFalloffPower = 0.76;
			gTree.mProperties.mClumpMax = 0.53;
			gTree.mProperties.mClumpMin = 0.419;
			gTree.mProperties.mBranchFactor = 3.4;
			gTree.mProperties.mDropAmount = -0.16;
			gTree.mProperties.mGrowAmount = 0.128;
			gTree.mProperties.mSweepAmount = 0.01;
			gTree.mProperties.mMaxRadius = 0.168;
			gTree.mProperties.mClimbRate = 0.472;
			gTree.mProperties.mTrunkKink = 0.06;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.835;
			gTree.mProperties.mRadiusFalloffRate = 0.73;
			gTree.mProperties.mTwistRate = 1.29;
			gTree.mProperties.mTrunkLength = 2.2;
		}
		ImGui::Image((ImTextureID)tex_preset[6], previewsize);

		if (ImGui::Button("Load preset 7"))
		{
			gTree.mProperties.mSeed = 152;
			gTree.mProperties.mSegments = 8;
			gTree.mProperties.mLevels = 5;
			gTree.mProperties.mVMultiplier = 1.16;
			gTree.mProperties.mTwigScale = 0.39;
			gTree.mProperties.mInitialBranchLength = 0.49;
			gTree.mProperties.mLengthFalloffFactor = 0.85;
			gTree.mProperties.mLengthFalloffPower = 0.99;
			gTree.mProperties.mClumpMax = 0.454;
			gTree.mProperties.mClumpMin = 0.454;
			gTree.mProperties.mBranchFactor = 3.2;
			gTree.mProperties.mDropAmount = 0.09;
			gTree.mProperties.mGrowAmount = 0.235;
			gTree.mProperties.mSweepAmount = 0.051;
			gTree.mProperties.mMaxRadius = 0.105;
			gTree.mProperties.mClimbRate = 0.322;
			gTree.mProperties.mTrunkKink = 0;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.964;
			gTree.mProperties.mRadiusFalloffRate = 0.73;
			gTree.mProperties.mTwistRate = 1.5;
			gTree.mProperties.mTrunkLength = 2.25;
		}
		ImGui::Image((ImTextureID)tex_preset[7], previewsize);

		if (ImGui::Button("Load preset 8"))
		{
			gTree.mProperties.mSeed = 267;
			gTree.mProperties.mSegments = 8;
			gTree.mProperties.mLevels = 4;
			gTree.mProperties.mVMultiplier = 0.96;
			gTree.mProperties.mTwigScale = 0.7;
			gTree.mProperties.mInitialBranchLength = 0.26;
			gTree.mProperties.mLengthFalloffFactor = 0.94;
			gTree.mProperties.mLengthFalloffPower = 0.7;
			gTree.mProperties.mClumpMax = 0.556;
			gTree.mProperties.mClumpMin = 0.404;
			gTree.mProperties.mBranchFactor = 3.5;
			gTree.mProperties.mDropAmount = -0.15;
			gTree.mProperties.mGrowAmount = 0.28;
			gTree.mProperties.mSweepAmount = 0.01;
			gTree.mProperties.mMaxRadius = 0.139;
			gTree.mProperties.mClimbRate = 0.419;
			gTree.mProperties.mTrunkKink = 0.093;
			gTree.mProperties.mTreeSteps = 5;
			gTree.mProperties.mTaperRate = 0.947;
			gTree.mProperties.mRadiusFalloffRate = 0.73;
			gTree.mProperties.mTwistRate = 3.32;
			gTree.mProperties.mTrunkLength = 2.2;
		}
	}

	if (ImGui::TreeNode("Twig texture"))
	{
		int i;
		for (i = 0; i < gTwigTextureCount; i++)
		{
			ImGui::Image((ImTextureID)gTwigTexture[i], previewsize);
			if (ImGui::Button(gTwigTextureName[i]))
			{
				gTwigTextureIndex = i;
				tex_twig = gTwigTexture[i];
			}
		}
	}

	if (ImGui::TreeNode("Trunk texture"))
	{
		int i;
		for (i = 0; i < gTrunkTextureCount; i++)
		{
			ImGui::Image((ImTextureID)gTrunkTexture[i], previewsize);
			if (ImGui::Button(gTrunkTextureName[i]))
			{
				gTrunkTextureIndex = i;
				tex_trunk = gTrunkTexture[i];
			}
		}
	}

	ImGui::End();
}

void lazyTextureLoad();

void draw_screen()
{
    int tick = SDL_GetTicks();

	lazyTextureLoad();

	if (gTwigTextureIndex >= 0)
	{
		if (gTwigTextureIndex >= gTwigTextureCount)
			gTwigTextureIndex = gTwigTextureCount - 1;
		if (gTwigTextureIndex >= 0)
			tex_twig = gTwigTexture[gTwigTextureIndex];
	}

	if (gTrunkTextureIndex >= 0)
	{
		if (gTrunkTextureIndex >= gTrunkTextureCount)
			gTrunkTextureIndex = gTrunkTextureCount - 1;
		if (gTrunkTextureIndex >= 0)
			tex_trunk = gTrunkTexture[gTrunkTextureIndex];
	}

	if (gUIState.not_imgui_mousedown)
	{
		float xdelta = gUIState.mousedownx - gUIState.mousex;
		gUIState.mousedownx = gUIState.mousex;
		gCamRotate.x += xdelta * -0.01f;

		float ydelta = gUIState.mousedowny - gUIState.mousey;
		gUIState.mousedowny = gUIState.mousey;
		gCamRotate.y += ydelta * -0.01f;
		if (gCamRotate.y < 0.02)
			gCamRotate.y = 0.02;
		if (gCamRotate.y > 1.5)
			gCamRotate.y = 1.5;
	}

	if (!inEditor())
	{
		if (gUIState.scroll)
		{
			gCamRotate.z += -gUIState.scroll;
			if (gCamRotate.z < 5)
				gCamRotate.z = 5;
			if (gCamRotate.z > 40)
				gCamRotate.z = 40;
			gUIState.scroll = 0;
		}
	}

	draw_imgui();

    int i;    

    if (tick - gLastTick > TICK_TIMEWARP) 
        gLastTick = tick;

    if (gLastTick >= tick)
    {
        SDL_Delay(1);
        return;
    }

    while (gLastTick < tick)
    {
        physics_tick(gLastTick);

        gLastTick += 1000 / PHYSICS_FPS;
    }

    ////////////////////////////////////
    // Rendering
    ////////////////////////////////////

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(gSkyColor[0],gSkyColor[1],gSkyColor[2],1.0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if (gWireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	if (propertiesChanged())
	{
		gTree.generate();
		glBindBuffer(GL_ARRAY_BUFFER, gVertVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mVertCount * sizeof(float) * 3, gTree.mVert, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gNormalVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mVertCount * sizeof(float) * 3, gTree.mNormal, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gUVVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mVertCount * sizeof(float) * 2, gTree.mUV, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gTwigVertVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mTwigVertCount * sizeof(float) * 3, gTree.mTwigVert, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gTwigNormalVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mTwigVertCount * sizeof(float) * 3, gTree.mTwigNormal, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gTwigUVVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mTwigVertCount * sizeof(float) * 2, gTree.mTwigUV, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gFaceVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mFaceCount * sizeof(int) * 3, gTree.mFace, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, gTwigFaceVBO);
		glBufferData(GL_ARRAY_BUFFER, gTree.mTwigFaceCount * sizeof(int) * 3, gTree.mTwigFace, GL_STATIC_DRAW);
	}
	
	int pass;
	for (pass = 0; pass < 2; pass++)
	{
		if (pass == 0)
		{
			setup_shadow();
		}
		else
		{
			setup_rendering(tick);
		}

		if (pass != 0)
		{
			draw_floor(gBaseShader);
		}

		prep_draw_tree();

		if (gForestMode)
		{
			srand(0);
			for (i = 0; i < 100; i++)
			{
				float xofs = (rand() % 1000) - 500;
				float zofs = (rand() % 1000) - 500;
				float l = sqrt(xofs*xofs + zofs*zofs);
				xofs /= l;
				zofs /= l;
				l = (rand() % 1000) * 0.025f;
				xofs *= l;
				zofs *= l;
				glm::mat4 temp = mat_modelview;

				glm::mat4 trans = glm::translate(glm::mat4(), glm::vec3(xofs, 0, zofs));
				mat_modelview = mat_modelview * trans;
				draw_tree(pass ? gBaseShader : gShadowpassShader);
				mat_modelview = temp;
			}
		}
		else
		{
			draw_tree(pass ? gBaseShader : gShadowpassShader);
		}

		finish_draw_tree();

		if (gTwigMode)
		{
			glm::mat4 orig;
			if (pass == 0)
			{
				// To reduce surface acne on the twigs, move them closer to the light on shadow pass..
				orig = mat_modelview;
				glm::vec3 xlate = glm::normalize(gLightDir) * 0.5f;
				mat_modelview = glm::translate(mat_modelview, xlate);
			}
			prep_draw_twig();

			if (gForestMode)
			{
				srand(0);
				for (i = 0; i < 100; i++)
				{
					float xofs = (rand() % 1000) - 500;
					float zofs = (rand() % 1000) - 500;
					float l = sqrt(xofs*xofs + zofs*zofs);
					xofs /= l;
					zofs /= l;
					l = (rand() % 1000) * 0.025f;
					xofs *= l;
					zofs *= l;
					glm::mat4 temp = mat_modelview;

					glm::mat4 trans = glm::translate(glm::mat4(), glm::vec3(xofs, 0, zofs));
					mat_modelview = mat_modelview * trans;
					draw_twig(pass ? gBaseShader : gShadowpassShader);
					mat_modelview = temp;
				}
			}
			else
			{
				draw_twig(pass ? gBaseShader : gShadowpassShader);
			}

			finish_draw_twig();
			if (pass == 0)
			{
				mat_modelview = orig;
			}
		}
	}

#ifdef DEBUG_SHADOWMAPS
	shadowmap_debug();
#endif

	ImGui::Render();

    //SDL_Delay(10);
	SDL_GL_SwapBuffers();
}


char * mystrdup(char * src, int ofs)
{
	int l = strlen(src) - ofs;
	char * buf = new char[l + 1];
	memcpy(buf, src + ofs, l);
	buf[l] = 0;
	return buf;
}


int imageExists(char *aBaseFilename, int aTwig)
{
	
	char *ext[] = { "TGA", "PNG", "JPG", "JPEG", "BMP", "PSD", "GIF", "HDR", "PIC", "PPM", "PGM"};
	char temp[2048];
	int i;
	for (i = 0; i < sizeof(ext)/sizeof(char*); i++)
	{
		sprintf(temp, "%s.%s", aBaseFilename, ext[i]);
		FILE * f = fopen(temp, "rb");
		if (f)
		{
			fclose(f);
			if (aTwig)
			{
				if (gTwigTextureCount < MAXTEXTURES)
				{
					gTwigTexture[gTwigTextureCount] = load_texture(temp);
					gTwigTextureName[gTwigTextureCount] = mystrdup(temp, 10);
					gTwigTextureCount++;
					progress();
				}
			}
			else
			{
				if (gTrunkTextureCount < MAXTEXTURES)
				{
					gTrunkTexture[gTrunkTextureCount] = load_texture(temp, 0);
					gTrunkTextureName[gTrunkTextureCount] = mystrdup(temp, 11);
					gTrunkTextureCount++;
					progress();
				}
			}
		}
	}
	return 0;
}

void findtextures(char *aBaseDir, int aTwig)
{
	WIN32_FIND_DATAA fdFile;
	HANDLE h = NULL;

	char path[2048];
	sprintf(path, "%s\\*.*", aBaseDir);

	h = FindFirstFileA(path, &fdFile);

	if (h == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(fdFile.cFileName, ".") != 0 &&
			strcmp(fdFile.cFileName, "..") != 0)
		{
			if (fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
			{				
				sprintf(path, "%s\\%s\\diffuse", aBaseDir, fdFile.cFileName);
				imageExists(path, aTwig);
			}
		}
	} 
	while (FindNextFileA(h, &fdFile)); 

	FindClose(h);

}


void initGraphicsAssets()
{
	// framework will take care of restoring textures on resize
	tex_twig = load_texture("data/twig.png");
	progress();
	tex_trunk = load_texture("data/bark.jpg", 0);
	progress();
	tex_floor = load_texture("data/floor.png", 0);
	progress();

	tex_preset[0] = load_texture("data/preset1.jpg");
	progress();
	tex_preset[1] = load_texture("data/preset2.jpg");
	progress();
	tex_preset[2] = load_texture("data/preset3.jpg");
	progress();
	tex_preset[3] = load_texture("data/preset4.jpg");
	progress();
	tex_preset[4] = load_texture("data/preset5.jpg");
	progress();
	tex_preset[5] = load_texture("data/preset6.jpg");
	progress();
	tex_preset[6] = load_texture("data/preset7.jpg");
	progress();
	tex_preset[7] = load_texture("data/preset8.jpg");


	findtextures("data\\twig", 1);
	findtextures("data\\trunk", 0);

	// keep shader sources in memory in case we need to re-build them on resize
	gBaseShader.init("data/base.vs", "data/base.fs");
	gShadowpassShader.init("data/shadowpass.vs", "data/shadowpass.fs");
#ifdef DEBUG_SHADOWMAPS
	gShadowDebugShader.init("data/shadowpass.vs", "data/shadowdebug.fs");
#endif

	progress();
	init_gl_resources();
	progress();
}


int main(int argc, char** args)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) 
    {
        fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(0);
    }

    initvideo(argc);

	// set window title
	SDL_WM_SetCaption(TITLE " - http://iki.fi/sol/", NULL);

	progress();

	InitImGui();

	progress();
	initGraphicsAssets();

	// For imgui - Enable keyboard repeat to make sliders more tolerable
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	// For imgui - Enable keyboard UNICODE processing for the text field.
	SDL_EnableUNICODE(1);

   

//	SDL_SetCursor(load_cursor("cursor_scissors.png",0,0));
    
    while (1) 
    {
        process_events();
        draw_screen();
    }

    return 0;
}
