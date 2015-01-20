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
