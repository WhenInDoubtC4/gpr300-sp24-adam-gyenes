#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../ew/transform.h"

class KNode
{
public:
	KNode(glm::vec3 localPosition = glm::vec3(0.f), glm::quat localRotation = glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3 localScale = glm::vec3(1.f));
	~KNode();

	glm::mat4 getLocalTransformMatrix() const;
	glm::mat4 getGlobalTransformMatrix() const;

	void attach(KNode* target);
	void detach();

	static void solveFKRecursive(KNode* start);

	void setLocalPosition(glm::vec3 localPosition);
	void setLocalRotation(glm::quat localRotation);
	void setLocalRotation(glm::vec3 localRotation);
	void setLocalScale(glm::vec3 localScale);

private:
	glm::vec3 _localPosition;
	glm::quat _localRotation;
	glm::vec3 _localScale;

	glm::mat4 _globalTransform;

	KNode* _parent = nullptr;
	std::vector<KNode*> _children;
};