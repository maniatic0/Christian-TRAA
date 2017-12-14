#include "enemyPlane.h"

#include "missile.h"

#include "core/helpers.hpp"
#include "core/utils.h"

#include <vector>
#include <cstdlib>
#include <cmath>

EnemyPlane::EnemyPlane(GLuint program, std::function<void(GLuint)> const & set_uniforms)
{
	if (!loaded)
	{
		loaded = true;
		EnemyPlane::geometry.reserve(2);
		EnemyPlane::geometry.push_back(bonobo::loadObjects("T-50.obj"));
		EnemyPlane::geometry.push_back(bonobo::loadObjects("mig21C.obj"));
		EnemyPlane::plane_textures.reserve(5);
		EnemyPlane::plane_textures.push_back(bonobo::loadTexture2D("T-50-2.png"));
		EnemyPlane::plane_textures.push_back(bonobo::loadTexture2D("T-50-3.png"));
		EnemyPlane::plane_textures.push_back(bonobo::loadTexture2D("T-50-4.png"));
		EnemyPlane::plane_textures.push_back(bonobo::loadTexture2D("T-50-5.png"));
		EnemyPlane::plane_textures.push_back(bonobo::loadTexture2D("mig21C.png"));
	}
	radius = 7.0f;
	modelNode = new Node();
	Node *part;
	int geo_type = rand() % EnemyPlane::geometry.size();
	int cammo_type;
	if (geo_type == 1)
	{
		cammo_type = EnemyPlane::plane_textures.size() - 1;
	}
	else
	{
		cammo_type = rand() % (EnemyPlane::plane_textures.size() - 1);
	}

	for (size_t i = 0; i < EnemyPlane::geometry[geo_type].size(); i++)
	{
		part = new Node();
		part->set_geometry(EnemyPlane::geometry[geo_type][i]);
		part->add_texture("diffuse_texture", EnemyPlane::plane_textures[cammo_type], GL_TEXTURE_2D);
		part->set_program(program, set_uniforms);
		modelNode->add_child(part);
	}
	if (geo_type == 0)
	{
		modelNode->rotate_y(bonobo::DEG2RAD * -90.0f);
		modelNode->set_translation(glm::vec3(0.0f, -10.0f, 0.0f));
	}
	else
	{
		modelNode->rotate_y(bonobo::DEG2RAD * 90.0f);
		modelNode->set_translation(glm::vec3(0.0f, -5.0f, 0.0f));
	}
	
	this->setLimits(50.0f, 20.0f, 5.0f, 5.0f);
	this->add_child(modelNode);
}

EnemyPlane::~EnemyPlane()
{
	delete(modelNode);
}

void EnemyPlane::collide(RigidBody * other)
{
	if (other == EnemyPlane::player)
	{
		deactivate();
		other->life--;
	}
}

void EnemyPlane::update(InputHandler * inputHandler, const float & dt)
{
	reloadTimeAcc += dt;
	temp = center - getPosition();
	if (std::abs(temp.x) > boxSize.x + 1.0f || std::abs(temp.y) > boxSize.y + 1.0f || life <= 0)
	{
		deactivate();
	}

	if (reloadTimeAcc > EnemyPlane::reloadTime)
	{
		reloadTimeAcc = 0.0f;
		temp = EnemyPlane::player->getPosition();
		if (vel.x >= 0.0f)
		{
			temp -= getPosition();
			if (temp.x >= 0.0f)
			{
				temp /= bonobo::magnitude(temp);
				Missile::shootMissile(static_cast<RigidBody *>(this), getPosition(), temp * missileSpeed, &EnemyPlane::playerTargetList);
			}
		}
		else
		{
			temp -= getPosition();
			if (temp.x <= 0.0f)
			{
				temp /= bonobo::magnitude(temp);
				Missile::shootMissile(static_cast<RigidBody *>(this), getPosition(), temp * missileSpeed, &EnemyPlane::playerTargetList);
			}
		}
	}

}

void EnemyPlane::shoot(glm::vec3 pos, glm::vec3 new_dir, float new_vel)
{
	set_translation(pos);
	life = (rand() % 3) + 1;
	missileSpeed = EnemyPlane::randomNumber(new_vel, 100.0f);
	vel = new_dir * new_vel;
	acc = new_dir * new_vel / 10.0f;
	auto const angle_z = glm::atan(new_dir.y, new_dir.x);
	set_rotation_z(angle_z);
	if (new_dir.x < 0.0f)
	{
		set_rotation_x(bonobo::pi);
	}
	rotate_x(bonobo::pi/2.0f * randomNumber(0.0f, 1.0f));
	active = true;
}

void EnemyPlane::deactivate()
{
	active = false;
}

void EnemyPlane::preloadPlanes(Node * group, RigidBody * new_player, unsigned int amount, GLuint program, std::function<void(GLuint)> const & set_uniforms)
{
	player = new_player;
	playerTargetList.emplace(new_player);
	preloadedEnemyPlanes.reserve(amount);
	EnemyPlane * new_plane;
	for (size_t i = 0; i < amount; i++)
	{
		new_plane = new EnemyPlane(program, set_uniforms);
		new_plane->deactivate();
		group->add_child(new_plane);
		preloadedEnemyPlanes.push_back(new_plane);
	}
	readyToShoot = 0;
}

void EnemyPlane::shootPlanes(const float & dt)
{
	planeReload += dt;
	if (planeReload <= planeReloadTime)
	{
		return;
	}
	planeReload = 0.0f;
	auto side = rand() % 2 == 1 ? boxSize.x : -boxSize.x;
	auto height = randomNumber(-boxSize.y, boxSize.y);
	auto pos = center + glm::vec3(side, height, 0.0f);
	auto dir = player->getPosition() - pos;
	preloadedEnemyPlanes[readyToShoot]->shoot(pos, dir / bonobo::magnitude(dir), EnemyPlane::randomNumber(20.0f, 50.0f));
	readyToShoot++;
	readyToShoot = readyToShoot % preloadedEnemyPlanes.size();
}

float EnemyPlane::randomNumber(float LO, float HI)
{
	return LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO)));
}

void EnemyPlane::deactivateEverything()
{
	for (size_t i = 0; i < preloadedEnemyPlanes.size(); i++)
	{
		preloadedEnemyPlanes[i]->active = false;
	}
}


float EnemyPlane::reloadTime = 2.0f;

float EnemyPlane::planeReload = 0.0f;

float EnemyPlane::planeReloadTime = 3.0f;

glm::vec3 EnemyPlane::boxSize = glm::vec3(400.0f, 100.0f, 0.0f);

glm::vec3 EnemyPlane::center = glm::vec3(0.0f, 100.0f, 0.0f);

std::unordered_set<RigidBody *> EnemyPlane::playerTargetList;

RigidBody *EnemyPlane::player;

std::vector<std::vector<bonobo::mesh_data>> EnemyPlane::geometry;

std::vector<GLuint> EnemyPlane::plane_textures;

std::vector<EnemyPlane *> EnemyPlane::preloadedEnemyPlanes;

int EnemyPlane::readyToShoot;
bool EnemyPlane::loaded = false;
