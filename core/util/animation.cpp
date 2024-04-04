#include "animation.h"

Animation::Animation(KNode* node)
	: _node(node)
{

}

void Animation::addKeyframe(float time, ew::Transform transform)
{
	_keyframes.push_back(std::make_pair(time, transform));
	_areKeyframesSorted = false;
}

void Animation::sortKeyframes()
{
	std::sort(_keyframes.begin(), _keyframes.end(), [](const Keyframe& a, const Keyframe& b)
		{
			return a.first < b.first;
		});

	_areKeyframesSorted = true;
}

float Animation::getLength() const
{
	if (!_areKeyframesSorted) sortKeyframes();

	return _keyframes.back().first;
}

ew::Transform Animation::getTransformAtNormalizedTime(float time) const
{
	if (!_areKeyframesSorted) sortKeyframes();

	bool found = false;
	Keyframe& keyframe0;
	Keyframe& keyframe1;

	for (int i = 0; i < _keyframes.size() - 1; i++)
	{
		//Find keyframe between specified interval
		if (_keyframes[i].first >= time && _keyframes[i + 1].first < time)
		{
			keyframe0 = _keyframes[i];
			keyframe1 = _keyframes[i + 1];

			found = true;
			break;
		}
	}

	assert(found);

	float alpha = (time - keyframe0.first) / (keyframe1.first - keyframe0.first);

	ew::Transform result;
	result.position = glm::lerp(keyframe0.second.position, keyframe1.second.position, alpha);
	result.rotation = glm::slerp(keyframe0.second.rotation, keyframe1.second.rotation, alpha);
	result.scale = glm::lerp(keyframe0.second.scale, keyframe1.second.scale, alpha);

	return result;
}
