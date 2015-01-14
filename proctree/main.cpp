/*
	proctree c++ port test / example
	Copyright (c) 2015 Jari Komppa

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgement in the product documentation would be
	appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

	NOTE: this license covers this example only, proctree.cpp has different license
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "proctree.h"

void benchmark()
{
	Proctree::Tree tree;
	printf("Start..\n");
	int n = 100000;
	int j;
	for (j = 0; j < 10; j++)
	{
		int start = GetTickCount();
		int i;
		for (i = 0; i < n; i++)
		{
			tree.generate();
		}
		int end = GetTickCount();
		float sec = (end - start) / 1000.0f;
		printf("%3.3fs (%3.3f trees per second)\n", sec, n / sec);
	}
}

void basic_use()
{
	// 1) Create the tree object

	Proctree::Tree tree;

	// 2) Change properties here for different kinds of trees
	/*
	tree.mProperties.mSeed = 7;
	tree.mProperties.mTreeSteps = 7;
	tree.mProperties.mLevels = 3;
	// etc.
	*/

	// 3) Call generate
	tree.generate();

	// 4) Use the data
	int i;

	for (i = 0; i < tree.mVertCount; i++)
	{
		printf("%+3.3f, %+3.3f, %+3.3f,    %+3.3f, %+3.3f, %+3.3f,    %+3.3f, %+3.3f,\n",
			tree.mVert[i].x, tree.mVert[i].y, tree.mVert[i].z,
			tree.mNormal[i].x, tree.mNormal[i].y, tree.mNormal[i].z,
			tree.mUV[i].u, tree.mUV[i].v);
	}
	printf("\n");
	exit(0);
	for (i = 0; i < tree.mTwigVertCount; i++)
	{
		printf("%+3.3f, %+3.3f, %+3.3f,    %+3.3f, %+3.3f, %+3.3f,    %+3.3f, %+3.3f,\n",
			tree.mTwigVert[i].x, tree.mTwigVert[i].y, tree.mTwigVert[i].z,
			tree.mTwigNormal[i].x, tree.mTwigNormal[i].y, tree.mTwigNormal[i].z,
			tree.mTwigUV[i].u, tree.mTwigUV[i].v);
	}

	// 5) Profit.

	// Note: You can change the properties and call generate to change the data, 
	// no need to delete the tree object in between.
}


int main(int parc, char **pars)
{
	//benchmark();
	basic_use();

}