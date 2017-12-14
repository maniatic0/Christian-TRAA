#include "missile.h"

#include "core/helpers.hpp"
#include "core/utils.h"

#include <vector>
#include <cstdlib>

GLuint Missile::missile_texture = 0u;
bonobo::mesh_data Missile::geometry;

std::vector<Missile *> Missile::preloadedMissiles;
int Missile::readyToShoot = 0;

float Missile::maxFlyTime = 30.0f;
glm::vec3 Missile::scale = glm::vec3(0.1f, 0.1f, 0.1f);

Missile::Missile() : RigidBody()
{
	if (Missile::missile_texture == 0u)
	{
		std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("AIM120D.blend.obj");
		Missile::geometry = objects.front();
		Missile::missile_texture = bonobo::loadTexture2D("AIM120D.png");
	}
	modelNode = new Node();
	modelNode->set_geometry(Missile::geometry);
	modelNode->add_texture("diffuse_texture", Missile::missile_texture, GL_TEXTURE_2D);
	modelNode->rotate_y(bonobo::DEG2RAD * -90.0f);
	modelNode->set_scaling(scale);
	this->setLimits(100.0f, 20.0f, 10.0f, 10.0f);
	this->add_child(modelNode);
	radius = 2.0f;
}

Missile::~Missile()
{
	delete(modelNode);
}

void Missile::set_program(GLuint program, std::function<void(GLuint)> const &set_uniforms)
{
	modelNode->set_program(program, set_uniforms);
}

void Missile::collide(RigidBody *other)
{
	if (targets == nullptr)
	{
		//deactivate();
		return;
	}

	auto const iter = targets->find(other);
	if (iter == targets->end())
	{
		return;
	}
	other->life -= 1;
	shooter->score++;
	deactivate();
}

void Missile::update(InputHandler* inputHandler, const float & dt)
{
	currentFlyTime += dt;
	if (currentFlyTime > Missile::maxFlyTime)
	{
		deactivate();
	}
}

void Missile::shoot(RigidBody * new_shooter, glm::vec3 pos, glm::vec3 new_vel, std::unordered_set<RigidBody*> *new_targets)
{
	shooter = new_shooter;
	set_translation(pos);
	vel = new_vel;
	acc = new_vel / 5.0f;
	auto const angle_x = glm::atan(vel.z, vel.y);
	auto const angle_y = -glm::atan(vel.z, vel.x);
	auto const angle_z = glm::atan(vel.y, vel.x);
	set_rotation_x(angle_x);
	set_rotation_y(angle_y);
	set_rotation_z(angle_z + (vel.x <= 0.0f? bonobo::pi : 0.0f));
	targets = new_targets;
	currentFlyTime = 0.0f;
	active = true;
}

void Missile::deactivate()
{
	active = false;
	targets = nullptr;
}

void Missile::preloadMissiles(Node * group, unsigned int amount, GLuint program, std::function<void(GLuint)> const &set_uniforms)
{
	preloadedMissiles.reserve(amount);
	Missile * new_missile;
	for (size_t i = 0; i < amount; i++)
	{
		new_missile = new Missile();
		new_missile->deactivate();
		new_missile->set_program(program, set_uniforms);
		group->add_child(new_missile);
		preloadedMissiles.push_back(new_missile);
	}
	readyToShoot = 0;
}

void Missile::shootMissile(RigidBody * new_shooter, glm::vec3 pos, glm::vec3 new_vel, std::unordered_set<RigidBody*>* new_targets)
{
	preloadedMissiles[readyToShoot]->shoot(new_shooter, pos, new_vel, new_targets);
	readyToShoot++;
	readyToShoot = readyToShoot % preloadedMissiles.size();
}

void Missile::deactivateEverything()
{
	for (size_t i = 0; i < preloadedMissiles.size(); i++)
	{
		preloadedMissiles[i]->active = false;
	}
}
