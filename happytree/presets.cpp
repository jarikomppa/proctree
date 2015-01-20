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

void loadpreset(int aPreset)
{
	switch (aPreset)
	{
	case 0:
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
		break;
	case 1:
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
		break;
	case 2:
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
		break;
	case 3:
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
		break;
	case 4:
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
		break;
	case 5:
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
		break;
	case 6:
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
		break;
	case 7:
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