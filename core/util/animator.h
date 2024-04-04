#pragma once

#include "animation.h"

class Animator
{
public:
	Animator() {};
	Animator(std::initializer_list<const Animation&>& initAnimations);

	void addAnimation(const Animation& anim);
	void updateAll(float deltaTime);

private:
	float _time = 0.f;
	std::vector<Animation*> _animations;
};