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

void exporth()
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

void exportobj()
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

int loadcustomtexture(int &aTexHandle, int aClamp)
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
		aTexHandle = load_texture(temp, aClamp);
		return 1;
	}
	return 0;
}

void loadproject()
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

void saveproject()
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
