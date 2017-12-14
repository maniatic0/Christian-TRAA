#pragma once

#include "core/node.hpp"
#include "core/Bonobo.h"
#include <glm/glm.hpp>
#include <unordered_set>
#include <core/Window.h>

class RigidBody : public Node
{
public:
	float radius = 1.0f;
	int life = 5;
	int score = 0;
	glm::mat4x4 currentWorldMatrix = glm::mat4();
	glm::vec3 vel = glm::vec3();
	glm::vec3 acc = glm::vec3();
	glm::vec3 angVel = glm::vec3();
	glm::vec3 angAcc = glm::vec3();
	glm::mat4x4 Update(const glm::mat4x4& parentWorldMatrix, const float& dt);
	void setLimits(const float maxVel, const float maxAcc, const float maxAngVel, const float maxAngAcc);
	glm::vec3 getPosition();
	glm::vec3 getFront();
	glm::vec3 getUp();
	glm::vec3 getRight();
	RigidBody();
	~RigidBody();
	virtual void collide(RigidBody * other);
	static void checkCollisions();
	static std::unordered_set<RigidBody *>* getRigidBodyList();
private:
	float _maxVel = 0.0f;
	float _maxAcc = 0.0f;
	float _maxAngVel = 0.0f;
	float _maxAngAcc = 0.0f;
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	static std::unordered_set<RigidBody *> collisionList;
};
