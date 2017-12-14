#include "rigidBody.h"

#include "core/helpers.hpp"

std::unordered_set<RigidBody *> RigidBody::collisionList;

void RigidBody::setLimits(const float maxVel, const float maxAcc, const float maxAngVel, const float maxAngAcc) {
	_maxVel = maxVel;
	_maxAcc = maxAcc;
	_maxAngVel = maxAngVel;
	_maxAngAcc = maxAngAcc;
}

glm::vec3 RigidBody::getPosition() {
	return position;
}

glm::vec3 RigidBody::getFront() {
	return front;
}

glm::vec3 RigidBody::getUp() {
	return up;
}

glm::vec3 RigidBody::getRight() {
	return right;
}

RigidBody::RigidBody() : Node()
{
	RigidBody::collisionList.emplace(this);
}

RigidBody::~RigidBody()
{
	RigidBody::collisionList.erase((this));
}

void RigidBody::collide(RigidBody *other)
{
	return;
}

void RigidBody::checkCollisions()
{
	auto current_iter = RigidBody::collisionList.begin();
	auto other_iter = current_iter;
	auto sqrmagnitude = 0.0f;
	auto sum_radius = 0.0f;

	while (current_iter != RigidBody::collisionList.end())
	{
		if (!(*current_iter)->active)
		{
			current_iter++;
			continue;
		}

		other_iter = std::next(current_iter, 1);
		while (other_iter != RigidBody::collisionList.end()) {
			if (!(*other_iter)->active)
			{
				other_iter++;
				continue;
			}
			sqrmagnitude = bonobo::sqrMagnitude((*other_iter)->position - (*current_iter)->position);
			sum_radius = (*other_iter)->radius + (*current_iter)->radius;

			if (sqrmagnitude < sum_radius * sum_radius)
			{
				(*current_iter)->collide((*other_iter));
				(*other_iter)->collide((*current_iter));
			}

			other_iter++;
		}

		current_iter++;
	}

}

std::unordered_set<RigidBody*>* RigidBody::getRigidBodyList()
{
	return &collisionList;
}


glm::mat4x4 RigidBody::Update(const glm::mat4x4& parentWorldMatrix, const float& dt) {
	rotate_x(angVel.x);
	rotate_y(angVel.y);
	rotate_z(angVel.z);
	auto const sqrAngAccMag = bonobo::sqrMagnitude(angAcc);
	if (sqrAngAccMag > _maxAngAcc * _maxAngAcc)
	{
		angAcc *= _maxAngAcc / std::sqrt(sqrAngAccMag);
	}


	angVel += angAcc *dt;
	auto const sqrAngVelMag = bonobo::sqrMagnitude(angVel);
	if (sqrAngVelMag > _maxAngVel * _maxAngVel)
	{
		angVel *= _maxAngVel / std::sqrt(sqrAngVelMag);
	}


	translate(glm::vec4(vel, 0.0f) * dt);
	auto const sqrAccMag = bonobo::sqrMagnitude(acc);
	if (sqrAccMag > _maxAcc * _maxAcc)
	{
		acc *= _maxAcc / std::sqrt(sqrAccMag);
	}


	vel += acc *dt;
	auto const sqrVelMag = bonobo::sqrMagnitude(vel);
	if (sqrVelMag > _maxVel * _maxVel)
	{
		vel *= _maxVel / std::sqrt(sqrVelMag);
	}

	currentWorldMatrix = parentWorldMatrix * get_transform();

	auto temp = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	position = glm::vec3(currentWorldMatrix * temp);

	temp.x = 1.0f;
	temp.w = 0.0f;
	front = glm::vec3(currentWorldMatrix * temp);

	temp.y = 1.0f;
	temp.x = 0.0f;
	up = glm::vec3(currentWorldMatrix * temp);

	temp.z = 1.0f;
	temp.y = 0.0f;
	right = glm::vec3(currentWorldMatrix * temp);

	return currentWorldMatrix;
}
