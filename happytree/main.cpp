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
#include "AntTweakBar.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

//#define DEBUG_SHADOWMAPS

#define TITLE "HappyTree 20150114"

void init_gl_resources();

// How big a twick difference is considered 'time warp', i.e. skip the time
// (to avoid physics blowing up)
#define TICK_TIMEWARP 1000
// Physics ticks per second
#define PHYSICS_FPS 100

#define USE_PERFCOUNTERS

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

int tex_twig, tex_bark, tex_floor;

Shader gBaseShader, gShadowpassShader;
#ifdef DEBUG_SHADOWMAPS
Shader gShadowDebugShader;
#endif
int gKeyState = 0;
int gLastTick = 0;
Proctree::Tree gTree;

Proctree::Properties gOldProp(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);

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

TwBar *gTreeBar = 0;
TwBar *gCommandBar = 0;

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
		if (!TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION))
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
			gUIState.mousex = (event.motion.x * (float)DESIRED_WINDOW_WIDTH) / (float)gScreenWidth;
			gUIState.mousey = (event.motion.y * (float)DESIRED_WINDOW_HEIGHT) / (float)gScreenHeight;
            break;
		case SDL_MOUSEBUTTONDOWN:
			// update button down state if left-clicking
			if (event.button.button == 1)
            {
				gUIState.mousedown = 1;
			    gUIState.mousedownx = (event.motion.x * (float)DESIRED_WINDOW_WIDTH) / (float)gScreenWidth;
			    gUIState.mousedowny = (event.motion.y * (float)DESIRED_WINDOW_HEIGHT) / (float)gScreenHeight;
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
				gUIState.mousedown = 0;
			break;
        case SDL_QUIT:
			TwTerminate();
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
int gAnimateLight = 1;
float gSkyColor[3] = { 0.1, 0.1, 0.2 };
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

glm::vec2 gCamRotate = { 0, 0 };

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

	float d = 20 + gForestMode * 20;

	mat_proj = glm::ortho<float>(-d, d, -d, d, 10, 100);

	mat_modelview = glm::mat4();
	int tick = 0;
	mat_modelview *= glm::lookAt(
		gLightDir * 20.0f,
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0));

	calc_shadowmatrix();
}

void restore_viewport()
{
#ifdef DESIRED_ASPECT
	float aspect = DESIRED_ASPECT;
	if (((float)gScreenWidth / gScreenHeight) > aspect)
	{
		float realx = gScreenHeight * aspect;
		float extrax = gScreenWidth - realx;

		glViewport(extrax / 2, 0, realx, gScreenHeight);
	}
	else
	{
		float realy = gScreenWidth / aspect;
		float extray = gScreenHeight - realy;

		glViewport(0, extray / 2, gScreenWidth, realy);
	}
#else
	glViewport(0, 0, gScreenWidth, gScreenHeight);
#endif
}

void setup_rendering(int tick)
{
	restore_viewport();

	
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

	mat_proj = glm::perspective((float)M_PI * 60.0f / 360.0f, 4.0f / 3.0f, 1.0f, 1000.0f);

	mat_modelview = glm::mat4();

	float d = 20 + gForestMode * 20;
	glm::vec3 at = { cos(gCamRotate.x), cos(gCamRotate.y), sin(gCamRotate.x) };
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

	glBindTexture(GL_TEXTURE_2D, tex_bark);

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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

static void draw_screen()
{
    int tick = SDL_GetTicks();

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


	//gTree.mProperties.mSeed = tick;
	gTree.generate(); 

	if (propertiesChanged())
	{
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

	TwDraw();

    //SDL_Delay(10);
	SDL_GL_SwapBuffers();
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


#define CMD_SAVE (void*)0
#define CMD_LOAD (void*)1
#define CMD_LOADTRUNKTEXTURE (void*)2
#define CMD_LOADTWIGTEXTURE (void*)3
#define CMD_RESET1 (void*)4
#define CMD_RESET2 (void*)5
#define CMD_RESET3 (void*)6
#define CMD_RESET4 (void*)7
#define CMD_RESET5 (void*)8
#define CMD_RESET6 (void*)9
#define CMD_RESET7 (void*)10
#define CMD_RESET8 (void*)11
#define CMD_EXPORTOBJ (void*)12
#define CMD_EXPORTH (void*)13


void TW_CALL command(void *clientData)
{
	SDL_SysWMinfo sysinfo;
	SDL_VERSION(&sysinfo.version);
	SDL_GetWMInfo(&sysinfo);

	if (clientData == CMD_LOAD)
	{
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

	if (clientData == CMD_SAVE)
	{
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

	if (clientData == CMD_LOADTRUNKTEXTURE)
	{
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
			tex_bark = load_texture(temp, 0);
		}
	}

	if (clientData == CMD_LOADTWIGTEXTURE)
	{
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
		}
	}

	if (clientData == CMD_EXPORTOBJ)
	{
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

	if (clientData == CMD_EXPORTOBJ)
	{
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

	if (clientData == CMD_RESET1 ||
		clientData == CMD_RESET2 ||
		clientData == CMD_RESET3 ||
		clientData == CMD_RESET4 ||
		clientData == CMD_RESET5 ||
		clientData == CMD_RESET6 ||
		clientData == CMD_RESET7 ||
		clientData == CMD_RESET8)
	{
		if (1)//MessageBoxA(sysinfo.window, "Load preset?", "Confirmation", MB_OKCANCEL) == IDOK)
		{
			if (clientData == CMD_RESET1)
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
			if (clientData == CMD_RESET2)
			{
				gTree.mProperties.mSeed=861;
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
			if (clientData == CMD_RESET3)
			{
				gTree.mProperties.mSeed=152;
				gTree.mProperties.mSegments=6;
				gTree.mProperties.mLevels=5;
				gTree.mProperties.mVMultiplier=1.16;
				gTree.mProperties.mTwigScale=0.44;
				gTree.mProperties.mInitialBranchLength=0.49;
				gTree.mProperties.mLengthFalloffFactor=0.85;
				gTree.mProperties.mLengthFalloffPower=0.99;
				gTree.mProperties.mClumpMax=0.454;
				gTree.mProperties.mClumpMin=0.246;
				gTree.mProperties.mBranchFactor=3.2;
				gTree.mProperties.mDropAmount=0.09;
				gTree.mProperties.mGrowAmount=0.235;
				gTree.mProperties.mSweepAmount=0.01;
				gTree.mProperties.mMaxRadius=0.111;
				gTree.mProperties.mClimbRate=0.41;
				gTree.mProperties.mTrunkKink=0;
				gTree.mProperties.mTreeSteps=5;
				gTree.mProperties.mTaperRate=0.835;
				gTree.mProperties.mRadiusFalloffRate=0.73;
				gTree.mProperties.mTwistRate=2.06;
				gTree.mProperties.mTrunkLength=2.45;
			}
			if (clientData == CMD_RESET4)
			{
				gTree.mProperties.mSeed=499;
				gTree.mProperties.mSegments=8;
				gTree.mProperties.mLevels=5;
				gTree.mProperties.mVMultiplier=1;
				gTree.mProperties.mTwigScale=0.28;
				gTree.mProperties.mInitialBranchLength=0.5;
				gTree.mProperties.mLengthFalloffFactor=0.98;
				gTree.mProperties.mLengthFalloffPower=1.08;
				gTree.mProperties.mClumpMax=0.414;
				gTree.mProperties.mClumpMin=0.282;
				gTree.mProperties.mBranchFactor=2.2;
				gTree.mProperties.mDropAmount=0.24;
				gTree.mProperties.mGrowAmount=0.044;
				gTree.mProperties.mSweepAmount=0;
				gTree.mProperties.mMaxRadius=0.096;
				gTree.mProperties.mClimbRate=0.39;
				gTree.mProperties.mTrunkKink=0;
				gTree.mProperties.mTreeSteps=5;
				gTree.mProperties.mTaperRate=0.958;
				gTree.mProperties.mRadiusFalloffRate=0.71;
				gTree.mProperties.mTwistRate=2.97;
				gTree.mProperties.mTrunkLength=1.95;
			}
			if (clientData == CMD_RESET5)
			{
				gTree.mProperties.mSeed=267;
				gTree.mProperties.mSegments=8;
				gTree.mProperties.mLevels=4;
				gTree.mProperties.mVMultiplier=0.96;
				gTree.mProperties.mTwigScale=0.71;
				gTree.mProperties.mInitialBranchLength=0.12;
				gTree.mProperties.mLengthFalloffFactor=1;
				gTree.mProperties.mLengthFalloffPower=0.7;
				gTree.mProperties.mClumpMax=0.556;
				gTree.mProperties.mClumpMin=0.404;
				gTree.mProperties.mBranchFactor=3.5;
				gTree.mProperties.mDropAmount=0.18;
				gTree.mProperties.mGrowAmount=-0.108;
				gTree.mProperties.mSweepAmount=0.01;
				gTree.mProperties.mMaxRadius=0.139;
				gTree.mProperties.mClimbRate=0.419;
				gTree.mProperties.mTrunkKink=0.093;
				gTree.mProperties.mTreeSteps=5;
				gTree.mProperties.mTaperRate=0.947;
				gTree.mProperties.mRadiusFalloffRate=0.73;
				gTree.mProperties.mTwistRate=3.53;
				gTree.mProperties.mTrunkLength=1.75;
			}
			if (clientData == CMD_RESET6)
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
			if (clientData == CMD_RESET7)
			{
				gTree.mProperties.mSeed=152;
				gTree.mProperties.mSegments=8;
				gTree.mProperties.mLevels=5;
				gTree.mProperties.mVMultiplier=1.16;
				gTree.mProperties.mTwigScale=0.39;
				gTree.mProperties.mInitialBranchLength=0.49;
				gTree.mProperties.mLengthFalloffFactor=0.85;
				gTree.mProperties.mLengthFalloffPower=0.99;
				gTree.mProperties.mClumpMax=0.454;
				gTree.mProperties.mClumpMin=0.454;
				gTree.mProperties.mBranchFactor=3.2;
				gTree.mProperties.mDropAmount=0.09;
				gTree.mProperties.mGrowAmount=0.235;
				gTree.mProperties.mSweepAmount=0.051;
				gTree.mProperties.mMaxRadius=0.105;
				gTree.mProperties.mClimbRate=0.322;
				gTree.mProperties.mTrunkKink=0;
				gTree.mProperties.mTreeSteps=5;
				gTree.mProperties.mTaperRate=0.964;
				gTree.mProperties.mRadiusFalloffRate=0.73;
				gTree.mProperties.mTwistRate=1.5;
				gTree.mProperties.mTrunkLength=2.25;
			}
			if (clientData == CMD_RESET8)
			{
				gTree.mProperties.mSeed=267;
				gTree.mProperties.mSegments=8;
				gTree.mProperties.mLevels=4;
				gTree.mProperties.mVMultiplier=0.96;
				gTree.mProperties.mTwigScale=0.7;
				gTree.mProperties.mInitialBranchLength=0.26;
				gTree.mProperties.mLengthFalloffFactor=0.94;
				gTree.mProperties.mLengthFalloffPower=0.7;
				gTree.mProperties.mClumpMax=0.556;
				gTree.mProperties.mClumpMin=0.404;
				gTree.mProperties.mBranchFactor=3.5;
				gTree.mProperties.mDropAmount=-0.15;
				gTree.mProperties.mGrowAmount=0.28;
				gTree.mProperties.mSweepAmount=0.01;
				gTree.mProperties.mMaxRadius=0.139;
				gTree.mProperties.mClimbRate=0.419;
				gTree.mProperties.mTrunkKink=0.093;
				gTree.mProperties.mTreeSteps=5;
				gTree.mProperties.mTaperRate=0.947;
				gTree.mProperties.mRadiusFalloffRate=0.73;
				gTree.mProperties.mTwistRate=3.32;
				gTree.mProperties.mTrunkLength=2.2;
			}
		}
	}
}


void initGraphicsAssets()
{
	// framework will take care of restoring textures on resize
	tex_twig = load_texture("data/snappytwig.png");
	tex_bark = load_texture("data/bark.jpg", 0);
	tex_floor = load_texture("data/floor.png", 0);

	// keep shader sources in memory in case we need to re-build them on resize
	gBaseShader.init("data/base.vs", "data/base.fs");
	gShadowpassShader.init("data/shadowpass.vs", "data/shadowpass.fs");
#ifdef DEBUG_SHADOWMAPS
	gShadowDebugShader.init("data/shadowpass.vs", "data/shadowdebug.fs");
#endif

	init_gl_resources();
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

	TwInit(TW_OPENGL, NULL);
	TwWindowSize(gScreenWidth, gScreenHeight);
	gTreeBar = TwNewBar("Tree");
	TwAddVarRO(gTreeBar, "Trunk verts", TW_TYPE_INT32, &gTree.mVertCount, "group=stats");
	TwAddVarRO(gTreeBar, "Trunk faces", TW_TYPE_INT32, &gTree.mFaceCount, "group=stats");
	TwAddVarRO(gTreeBar, "Twig verts", TW_TYPE_INT32, &gTree.mTwigVertCount, "group=stats");
	TwAddVarRO(gTreeBar, "Twig faces", TW_TYPE_INT32, &gTree.mTwigFaceCount, "group=stats");

	TwAddVarRW(gTreeBar, "Random seed",          TW_TYPE_INT32, &gTree.mProperties.mSeed, " group=general min=0 max=10000");
	TwAddVarRW(gTreeBar, "Branch segments",      TW_TYPE_INT32, &gTree.mProperties.mSegments, " group=general step=2 min=4 max=32");
	TwAddVarRW(gTreeBar, "Branch levels", TW_TYPE_INT32, &gTree.mProperties.mLevels, " group=general min=3 step=1 max=32");
	TwAddVarRW(gTreeBar, "Trunk forks", TW_TYPE_INT32, &gTree.mProperties.mTreeSteps, " group=general min=0 step=1 max=32");
	TwAddVarRW(gTreeBar, "Texture V multiplier", TW_TYPE_FLOAT, &gTree.mProperties.mVMultiplier, " group=general min=0.01 step=0.01 max=10");
	TwAddVarRW(gTreeBar, "Twig scale",           TW_TYPE_FLOAT, &gTree.mProperties.mTwigScale, " group=general min=0.01 step=0.001 max=2");

	TwAddVarRW(gTreeBar, "Initial length",       TW_TYPE_FLOAT, &gTree.mProperties.mInitialBranchLength, " group=branching min=0.01 step=0.01 max=5");
	TwAddVarRW(gTreeBar, "Len falloff rate",     TW_TYPE_FLOAT, &gTree.mProperties.mLengthFalloffFactor, " group=branching min=0.01 step=0.01 max=1.5");
	TwAddVarRW(gTreeBar, "Len falloff power",    TW_TYPE_FLOAT, &gTree.mProperties.mLengthFalloffPower, " group=branching min=-2 step=0.01 max=2");
	TwAddVarRW(gTreeBar, "Max clumping",         TW_TYPE_FLOAT, &gTree.mProperties.mClumpMax, " group=branching min=0.01 step=0.01 max=10");
	TwAddVarRW(gTreeBar, "Min clumping",         TW_TYPE_FLOAT, &gTree.mProperties.mClumpMin, " group=branching min=0.01 step=0.01 max=10");
	TwAddVarRW(gTreeBar, "Symmetry",             TW_TYPE_FLOAT, &gTree.mProperties.mBranchFactor, " group=branching min=2 step=0.01 max=4");
	TwAddVarRW(gTreeBar, "Droop",                TW_TYPE_FLOAT, &gTree.mProperties.mDropAmount, " group=branching min=-2 step=0.01 max=2");
	TwAddVarRW(gTreeBar, "Growth",               TW_TYPE_FLOAT, &gTree.mProperties.mGrowAmount, " group=branching min=-4 step=0.01 max=4");
	TwAddVarRW(gTreeBar, "Sweep",                TW_TYPE_FLOAT, &gTree.mProperties.mSweepAmount, " group=branching min=-1 step=0.01 max=1");

	TwAddVarRW(gTreeBar, "Trunk radius",         TW_TYPE_FLOAT, &gTree.mProperties.mMaxRadius, " group=trunk min=0.01 step=0.001 max=0.5");
	TwAddVarRW(gTreeBar, "Radius falloff", TW_TYPE_FLOAT, &gTree.mProperties.mRadiusFalloffRate, " group=trunk min=0.1 step=0.01 max=1.0");
	TwAddVarRW(gTreeBar, "Climb rate", TW_TYPE_FLOAT, &gTree.mProperties.mClimbRate, " group=trunk min=0.01 step=0.01 max=1.0");
	TwAddVarRW(gTreeBar, "Kink",                 TW_TYPE_FLOAT, &gTree.mProperties.mTrunkKink, " group=trunk min=-2 step=0.01 max=2");
	TwAddVarRW(gTreeBar, "Taper rate",           TW_TYPE_FLOAT, &gTree.mProperties.mTaperRate, " group=trunk min=0.5 step=0.01 max=2");
	TwAddVarRW(gTreeBar, "Twists",               TW_TYPE_FLOAT, &gTree.mProperties.mTwistRate, " group=trunk min=0.01 step=0.01 max=10");
	TwAddVarRW(gTreeBar, "Trunk length",         TW_TYPE_FLOAT, &gTree.mProperties.mTrunkLength, " group=trunk min=0.01 step=0.01 max=5");

	TwDefine(" Tree size='250 500' position='10 10' ");

	gCommandBar = TwNewBar("Commands");
	TwAddButton(gCommandBar, "Save", command, CMD_SAVE, " group=project ");
	TwAddButton(gCommandBar, "Load", command, CMD_LOAD, " group=project ");
	TwAddButton(gCommandBar, "Load trunk texture", command, CMD_LOADTRUNKTEXTURE, " group=texture ");
	TwAddButton(gCommandBar, "Load twig texture", command, CMD_LOADTWIGTEXTURE, " group=texture ");
	TwAddButton(gCommandBar, "Load preset 1", command, CMD_RESET1, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 2", command, CMD_RESET2, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 3", command, CMD_RESET3, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 4", command, CMD_RESET4, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 5", command, CMD_RESET5, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 6", command, CMD_RESET6, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 7", command, CMD_RESET7, " group=presets ");
	TwAddButton(gCommandBar, "Load preset 8", command, CMD_RESET8, " group=presets ");
	TwAddButton(gCommandBar, "Mesh as WaveFront .obj", command, CMD_EXPORTOBJ, " group=export ");
	TwAddButton(gCommandBar, "Params as c++ .h", command, CMD_EXPORTH, " group=export ");
	TwAddVarRW(gCommandBar, "Forest mode", TW_TYPE_BOOL32, &gForestMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Enable textures", TW_TYPE_BOOL32, &gTextureMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Wireframe mode", TW_TYPE_BOOL32, &gWireframeMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Draw twigs", TW_TYPE_BOOL32, &gTwigMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Lighting", TW_TYPE_BOOL32, &gLightingMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Shadows", TW_TYPE_BOOL32, &gShadowMode, " group=rendering ");
	TwAddVarRW(gCommandBar, "Animate light", TW_TYPE_BOOL32, &gAnimateLight, " group=rendering ");
	TwAddVarRW(gCommandBar, "Sky color", TW_TYPE_COLOR3F, &gSkyColor, " group=rendering ");

	TwDefine(" Commands size='250 500' position='700 10' "); // 960-260 = 700

	initGraphicsAssets();

	// For imgui - Enable keyboard repeat to make sliders more tolerable
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	// For imgui - Enable keyboard UNICODE processing for the text field.
	SDL_EnableUNICODE(1);

   
    // set window title
    SDL_WM_SetCaption(TITLE " - http://iki.fi/sol/", NULL);

//	SDL_SetCursor(load_cursor("cursor_scissors.png",0,0));
    
    while (1) 
    {
        process_events();
        draw_screen();
    }

    return 0;
}
