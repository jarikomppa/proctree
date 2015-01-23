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
#include "happytree.h"

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

int gForestMode = 0;
int gTextureMode = 1;
int gTwigMode = 1;
int gWireframeMode = 0;
int gLightingMode = 1;
int gShadowMode = 1;
int gAnimateLight = 0;
float gSkyColor[3] = { 70 / 256.0f, 112 / 256.0f, 175 / 256.0f };
glm::vec3 gLightDir = { 1, 1, 0 };
glm::mat4 mat_proj;
glm::mat4 mat_modelview;
glm::mat4 mat_shadow;

glm::vec3 gCamRotate = { 0, 0.2, 20 };

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

int gKeyState = 0;
int gLastTick = 0;
Proctree::Tree gTree;

Proctree::Properties gOldProp(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

Shader gBaseShader, gShadowpassShader;
#ifdef DEBUG_SHADOWMAPS
Shader gShadowDebugShader;
#endif


int inEditor()
{
	float w = gScreenWidth / 3;
	if (w < 400) w = 400;
	if (gUIState.mousex < w && ImGui::IsMouseHoveringAnyWindow())
		return 1;

	w = gScreenWidth / 5;
	if (w < 200) w = 200;
	w = gScreenWidth - (gScreenWidth / 5);
	if (gUIState.mousex > w && ImGui::IsMouseHoveringAnyWindow())
		return 1;
	return 0;
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



void draw_imgui()
{
	UpdateImGui();

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
		CLAMP(gTree.mProperties.mSegments, 2, 32);	
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
			saveproject();
		}
		if (ImGui::Button("Load project"))
		{
			loadproject();
		}
	}

	if (ImGui::TreeNode("Custom textures"))
	{
		if (ImGui::Button("Load custom trunk texture"))
		{
			if (loadcustomtexture(tex_trunk, 0))
				gTrunkTextureIndex = -1;
		}
		if (ImGui::Button("Load custom twig texture"))
		{
			if (loadcustomtexture(tex_twig, 1))
				gTwigTextureIndex = -1;
		}
		if (ImGui::Button("Load custom floor texture"))
		{
			loadcustomtexture(tex_floor, 0);
		}
	}
	if (ImGui::TreeNode("Export"))
	{
		if (ImGui::Button("Export mesh as .obj"))
		{
			exportobj();
		}
		if (ImGui::Button("Export params as c++ .h"))
		{
			exporth();
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
	if (w < 200) w = 200;
	ImVec2 previewsize(w-50, w-50);

	ImGui::SetNextWindowPos({ gScreenWidth - w, 0 });
	ImGui::SetNextWindowSize({ w, (float)gScreenHeight });
	ImGui::Begin("Content browser", &yah, ImVec2(0, 0), 0.2f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);


	if (ImGui::TreeNode("Presets"))
	{	
		int i;
		for (i = 0; i < 8; i++)
		{
			ImGui::Image((ImTextureID)tex_preset[0], previewsize);
			char temp[80];
			sprintf(temp, "Load preset %d", i + 1);
			if (ImGui::Button("Load preset 1"))
			{
				loadpreset(i);
			}
		}
	}

	if (ImGui::TreeNode("Twig texture"))
	{
		if (gTwigTextureCount == 0)
		{
			ImGui::Text("No textures found");
			ImGui::Text("under data/twig/");
		}
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
		if (gTrunkTextureCount == 0)
		{
			ImGui::Text("No textures found");
			ImGui::Text("under data/trunk/");
		}
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

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
