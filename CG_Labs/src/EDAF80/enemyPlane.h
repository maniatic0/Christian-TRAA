#pragma once

#include "rigidBody.h"
#include "core/Bonobo.h"
#include "core/node.hpp"
#include <unordered_set>
#include <vector>


class EnemyPlane : public RigidBody
{
public:
	bool visible = true;
	Node *modelNode;
	glm::vec3 temp;
	float reloadTimeAcc = 0.0f;
	float missileSpeed = 1.0f;

	EnemyPlane(GLuint program, std::function<void(GLuint)> const & set_uniforms);
	~EnemyPlane();

	void collide(RigidBody *other);

	void update(InputHandler* inputHandler, const float& dt);

	void shoot(glm::vec3 pos, glm::vec3 new_dir, float new_vel);

	void deactivate();

	static float reloadTime;

	static float planeReload;

	static float planeReloadTime;

	static glm::vec3 boxSize;

	static glm::vec3 center;

	static std::vector<EnemyPlane *> preloadedEnemyPlanes;

	static RigidBody *player;

	static void preloadPlanes(Node *group, RigidBody *new_player, unsigned int amount, GLuint program, std::function<void(GLuint)> const &set_uniforms);

	static void shootPlanes(const float & dt);

	static float randomNumber(float LO, float HI);

	static void deactivateEverything();

private:
	static std::vector<std::vector<bonobo::mesh_data>> geometry;
	static std::vector<GLuint> plane_textures;
	static int readyToShoot;
	static std::unordered_set<RigidBody *> playerTargetList;
	static bool loaded;
};
