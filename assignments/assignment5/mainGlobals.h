#pragma once

#include "util/kNode.h"
#include "util/animation.h"
#include "util/animator.h"

constexpr glm::quat QUAT_DEG(float x, float y, float z)
{
	return glm::quat(glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z)));
}

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

struct Anims
{
	Animation* root;

	Animation* head;

	Animation* shoulder_r;

	Animation* shoulder_l;

	Animation* thigh_r;

	Animation* thigh_l;
};

AnimRig animRig;
Anims anims;

Animator mainAnimator;