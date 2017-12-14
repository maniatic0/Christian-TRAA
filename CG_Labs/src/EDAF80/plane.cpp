#include "plane.h"
#include "config.hpp"
#include "missile.h"
#include "enemyPlane.h"

#include "core/helpers.hpp"
#include "core/utils.h"
#include "core/InputHandler.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include "glm\gtx\rotate_vector.hpp"

#include <vector>
#include <cstdlib>

GLuint Plane::plane_texture = 0u;
std::vector<bonobo::mesh_data> Plane::geometry;

Plane::Plane(FPSCameraf *new_camera, GLuint program, std::function<void(GLuint)> const & set_uniforms) : RigidBody()
{
	if (Plane::plane_texture == 0u)
	{
		Plane::geometry = bonobo::loadObjects("T-50.obj");
		Plane::plane_texture = bonobo::loadTexture2D("T-50.png");
	}
	radius = 5.0f;
	life = 5;
	camera = new_camera;
	modelNode = new Node();
	Node *part;
	for (size_t i = 0; i < Plane::geometry.size(); i++)
	{
		part = new Node();
		part->set_geometry(Plane::geometry[i]);
		part->add_texture("diffuse_texture", Plane::plane_texture, GL_TEXTURE_2D);
		part->set_program(program, set_uniforms);
		modelNode->add_child(part);
	}
	modelNode->rotate_y(bonobo::DEG2RAD * -90.0f);
	this->setLimits(50.0f, 15.0f, 5.0f, 5.0f);
	this->add_child(modelNode);
}

Plane::~Plane()
{
	delete(modelNode);
}

void Plane::collide(RigidBody * other)
{
	//active = false;
}

void Plane::update(InputHandler* inputHandler, const float & dt)
{
	if (life <= 0)
	{
		active = false;
		return;
	}
	EnemyPlane::shootPlanes(dt);
	reloadAcc += dt;
	time += dt;
	auto pos = getPosition();
	auto front = getFront();
	auto up = getUp();
	auto right = getRight();

	if (inputHandler->GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
		vel.x =  - front.x * planeVel;
		set_rotation_x(0.5f);
	}

	else if (inputHandler->GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
		vel.x = front.x * planeVel;
		set_rotation_x(-0.5f);
	}

	else {
		vel.x = 0.0f;
		set_rotation_x(0.0f);
	}

	if (inputHandler->GetKeycodeState(GLFW_KEY_UP) & PRESSED) {
		vel.y = up.y * planeVel;
		set_rotation_z(0.3f);
	}

	else if (inputHandler->GetKeycodeState(GLFW_KEY_DOWN) & PRESSED) {
		vel.y = - up.y  * planeVel;
		set_rotation_z(-0.3f);
	}

	else {
		vel.y = 0.0f;
		set_rotation_z(0.0f);
	}

	if (reloadAcc > reloadTime && inputHandler->GetKeycodeState(GLFW_KEY_SPACE) & PRESSED) {
		reloadAcc = 0.0f;
		Missile::shootMissile(static_cast<RigidBody *>(this), pos, 50.0f*front, &enemies);
	}

}

void Plane::Revive()
{
	life = 5;
	time = 0.0f;
	score = 0;
	active = true;
}
