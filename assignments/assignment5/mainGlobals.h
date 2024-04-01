#pragma once

#include "util/kNode.h"

struct AnimRig
{
	KNode* root;

	KNode* torso;
	
	KNode* head;

	KNode* shoulder_r;
	KNode* arm_r;
	KNode* hand_r;

	KNode* shoulder_l;
	KNode* arm_l;
	KNode* hand_l;

	KNode* thigh_r;
	KNode* shin_r;
	KNode* foot_r;

	KNode* thigh_l;
	KNode* shin_l;
	KNode* foot_l;
};

AnimRig animRig;