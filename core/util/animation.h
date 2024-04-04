#pragma once

#include <utility>
#include <algorithm>

#include "../ew/transform.h"

#include "kNode.h"

typedef std::pair<float, ew::Transform> Keyframe;

class Animation
{
public:
	Animation(KNode* node);

	void addKeyframe(float time, ew::Transform transform);

private:
	friend class Animator;

	void sortKeyframes();
	float getLength() const;
	ew::Transform getTransformAtNormalizedTime(float time) const;

	KNode* _node;
	bool _areKeyframesSorted = false;
	std::vector<Keyframe> _keyframes;
};