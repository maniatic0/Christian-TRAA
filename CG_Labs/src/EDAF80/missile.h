#pragma once

#include "rigidBody.h"
#include "core/Bonobo.h"
#include "core/node.hpp"
#include <unordered_set>
#include <vector>


class Missile : public RigidBody
{
public:
	bool visible = true;
	float currentFlyTime = 0.0f;
	Node * modelNode;
	std::unordered_set<RigidBody *> *targets = nullptr;
	RigidBody *shooter;
	Missile();
	~Missile();
	//! \brief Set the program of this node.
	//!
	//! A node without a program will not render itself, but its children
	//! will be rendered if they have one.
	//!
	//! @param [in] program OpenGL shader program to use
	//! @param [in] set_uniforms function that will take as argument an
	//!             OpenGL shader program, and will setup that program's
	//!             uniforms
	void set_program(GLuint program, std::function<void(GLuint)> const& set_uniforms);

	void collide(RigidBody * other);

	void update(InputHandler* inputHandler, const float& dt);

	void shoot(RigidBody * new_shooter, glm::vec3 pos, glm::vec3 new_vel, std::unordered_set<RigidBody *> *new_targets);

	void deactivate();

	static float maxFlyTime;

	static void preloadMissiles(Node *group, unsigned int amount, GLuint program, std::function<void(GLuint)> const &set_uniforms);

	static void shootMissile(RigidBody * new_shooter, glm::vec3 pos, glm::vec3 new_vel, std::unordered_set<RigidBody *> *new_targets);

	static void deactivateEverything();
private:
	static bonobo::mesh_data geometry;
	static GLuint missile_texture;
	static std::vector<Missile *> preloadedMissiles;
	static glm::vec3 scale;
	static int readyToShoot;
};
