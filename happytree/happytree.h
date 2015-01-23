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
#include "shader.h"

#define TITLE "HappyTree 20150123"
#define ONCE(x) { static int __once = 1; if (__once) {x;} __once = 0; }
#define CLAMP(var,min,max) { if (var < (min)) var = (min); if (var > (max)) var = (max); }
#define RB_WIDTH 1024
#define RB_HEIGHT 1024
#define MAXTEXTURES 64
//#define DEBUG_SHADOWMAPS
// How big a twick difference is considered 'time warp', i.e. skip the time
// (to avoid physics blowing up)
#define TICK_TIMEWARP 1000
// Physics ticks per second
#define PHYSICS_FPS 100

extern char *gTwigTextureName[MAXTEXTURES];
extern int gTwigTexture[MAXTEXTURES];
extern int gTwigTextureCount;
extern char *gTrunkTextureName[MAXTEXTURES];
extern int gTrunkTexture[MAXTEXTURES];
extern int gTrunkTextureCount;
extern int gTwigTextureIndex;
extern int gTrunkTextureIndex;
extern int tex_twig, tex_trunk, tex_floor;
extern int tex_preset[8];

extern GLuint gVertVBO;
extern GLuint gNormalVBO;
extern GLuint gUVVBO;
extern GLuint gFaceVBO;
extern GLuint gTwigVertVBO;
extern GLuint gTwigNormalVBO;
extern GLuint gTwigUVVBO;
extern GLuint gTwigFaceVBO;

extern GLuint gFloorVertVBO;
extern GLuint gFloorNormalVBO;
extern GLuint gFloorUVVBO;

extern GLuint rb_shadowfbo, rb_shadow;

extern int gForestMode;
extern int gTextureMode;
extern int gTwigMode;
extern int gWireframeMode;
extern int gLightingMode;
extern int gShadowMode;
extern int gAnimateLight;
extern float gSkyColor[3];
extern glm::vec3 gLightDir;
extern glm::mat4 mat_proj;
extern glm::mat4 mat_modelview;
extern glm::mat4 mat_shadow;

extern glm::vec3 gCamRotate;


extern int gKeyState;
extern int gLastTick;
extern Proctree::Tree gTree;

extern Proctree::Properties gOldProp;

extern Shader gBaseShader, gShadowpassShader;
#ifdef DEBUG_SHADOWMAPS
extern Shader gShadowDebugShader;
#endif

void lazyTextureLoad();
void loadpreset(int aPreset);
void UpdateImGui();
void InitImGui();
void ImImpl_InitGL();
void exportobj();
void exporth();
int loadcustomtexture(int &aTexHandle, int aClamp);
void saveproject();
void loadproject();
void export_obj(char *aFilename);
void export_h(char *aFilename);
void load_htr(char *aFilename);
void save_htr(char *aFilename);
char * loadfile(char *aFilename, int &aLen);
void init_gl_resources();
void calc_shadowmatrix();
void setup_shadow();
void setup_rendering(int tick);
void draw_floor(Shader &shader);
void prep_draw_tree();
void finish_draw_tree();
void draw_tree(Shader &shader);
void prep_draw_twig();
void draw_twig(Shader &shader);
void finish_draw_twig();
#ifdef DEBUG_SHADOWMAPS
void shadowmap_debug();
#endif
