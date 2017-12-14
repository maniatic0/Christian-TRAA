#pragma once

#include "rigidBody.h"
#include "core/Bonobo.h"
#include "core/node.hpp"
#include "core/FPSCamera.h"

#include <unordered_set>


class Plane : public RigidBody
{
public:
	bool visible = true;
	float cameraDistance = 300.0f;
	float cameraHeight = 50.0f;
	float planeVel = 20.0f;
	float reloadTime = 0.3f;
	float reloadAcc = 0.0f;
	float time = 0.0f;
	FPSCameraf *camera;
	Node * modelNode;
	std::unordered_set<RigidBody *> enemies;
	Plane(FPSCameraf *new_camera,GLuint program, std::function<void(GLuint)> const & set_uniforms);
	~Plane();

	void collide(RigidBody * other);

	void update(InputHandler* inputHandler, const float& dt);

	void Revive();

private:
	static std::vector<bonobo::mesh_data> geometry;
	static GLuint plane_texture;
};
