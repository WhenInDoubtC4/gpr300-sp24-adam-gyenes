#include "animator.h"

Animator::Animator(std::initializer_list<const Animation&>& initAnimations)
{
	for (Animation& anim : initAnimations)
	{
		_animations.push_back(&anim);
	}
}

void Animator::addAnimation(const Animation& anim)
{
	_animations.push_back(&anim);
}

void Animator::updateAll(float deltaTime)
{
	for (Animation* anim : _animations)
	{
		float normalizedTime = _time % anim->getLength();
		ew::Transform currentTransform = anim->getTransformAtNormalizedTime(normalizedTime);

		anim->_node->setLocalTransform(currentTransform);
	}

	_time += deltaTime;
}
