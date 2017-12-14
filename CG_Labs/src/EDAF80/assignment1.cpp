#include "assignment1.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/opengl.hpp"
#include "core/utils.h"
#include "core/various.hpp"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <vector>


edaf80::Assignment1::Assignment1()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 1", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment1::~Assignment1()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment1::run()
{
	// Random Max Float for Random float numbers
	auto const RANDOM_FLOAT_MAX = 10000.0f;
	// Generate random float numbers between 0 and RANDOM_FLOAT_MAX
	const std::function<float(void)> random = [RANDOM_FLOAT_MAX]() {return static_cast <float> (std::rand()) / (static_cast <float> (RAND_MAX / RANDOM_FLOAT_MAX)); };

	// Get vector magnitude
	const std::function<float(glm::vec3)> magnitude = [](glm::vec3 const v) {
		auto const x = v.x * v.x;
		auto const y = v.y * v.y;
		auto const z = v.z * v.z;
		return std::sqrt(x + y + z);
	};

	// Load the sphere geometry
	auto const objects = bonobo::loadObjects("sphere.obj");
	if (objects.empty())
		return;
	auto const& sphere = objects.front();

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	auto const cameraSpeed = 10.0f;
	mCamera.mMovementSpeed = 0.25f * 12.0f * cameraSpeed;
	window->SetCamera(&mCamera);

	// Create the shader program
	auto shader = bonobo::createProgram("default.vert", "default.frag");
	if (shader == 0u) {
		LogError("Failed to load shader");
		return;
	}

	auto const DEG2RAD = 0.0174533f;

#pragma region Sun

	auto sun = Node();

	// Sun radius
	auto const sunRadius = 695700.0f; // km

#pragma region SunModel
	// Load the sun's texture
	auto sun_texture = bonobo::loadTexture2D("sunmap.png");

	auto sunModel = Node();
	// Sun Scale
	auto const sunScale = 100.0f;
	sunModel.set_scaling(glm::vec3(sunScale, sunScale, sunScale));
	sunModel.set_geometry(sphere);
	sunModel.set_program(shader, [](GLuint /*program*/) {});
	sunModel.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);

	const auto sunStartRotation = random();
	const auto sunRotationScale = 1.0f / 50.0f;

	sun.add_child(&sunModel);
#pragma endregion

	// Done
	// Todo: Attach a texture to the sun
	//

	auto world = Node();
	world.add_child(&sun);
#pragma endregion

#pragma region Earth
	// Earth

	auto earth = Node();

	// Sun earth ratio
	auto const earthSunRatio = 0.5f;

	// Earth radius
	auto const earthRadius = 6371.0f; // km

	// Sun earth distance
	auto const earthSunDistance = 149.6f * 1000000.0f;

	// Sun earth distance in sun radius
	auto const earthSunDistanceRatio = earthSunDistance / sunRadius;
	auto const earthOrbitFrecuency = 1.0f / 100.0f;
	auto const earthStartOrbitRotation = random();

	auto const earthStartRotation = random();
	auto const earthRotationScale = 1.0f / 1.0f;


#pragma region EarthTilt
	auto earthTilt = Node();
	auto const earthTilting = DEG2RAD * 23.44f;
	earthTilt.set_rotation_x(earthTilting);

	earth.add_child(&earthTilt);

#pragma region EarthModel
	// Load the earth's texture
	auto earth_texture = bonobo::loadTexture2D("earth_diffuse.png");

	auto earthModel = Node();
	earthModel.set_scaling(glm::vec3(earthSunRatio, earthSunRatio, earthSunRatio));
	earthModel.set_geometry(sphere);
	earthModel.set_program(shader, [](GLuint /*program*/) {});
	earthModel.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);

	earthTilt.add_child(&earthModel);
#pragma endregion

#pragma endregion

	sun.add_child(&earth);
	//
	// Todo: Create an Earth node
	//  
#pragma endregion

#pragma region Moon
	// Moon
	auto moon = Node();

	// Moon Moon ratio
	auto const moonEarthRatio = 0.07f;

	moon.set_scaling(glm::vec3(moonEarthRatio, moonEarthRatio, moonEarthRatio));

	auto const moonOrbitRotation = DEG2RAD * 20.0f; // original 5.14
	moon.set_rotation_x(moonOrbitRotation);

	auto const moonEarthDistance = 25.0f;
	auto const moonOrbitFrecuency = 1.0f / 5.0f;
	auto const moonStartOrbitRotation = random();

#pragma region MoonTilt
	auto moonTilt = Node();
	auto const moonTilting = - DEG2RAD * 6.68f;
	moonTilt.set_rotation_x(moonTilting);
	

#pragma region MoonModel
	auto moonModel = Node();

	// Load the Moon's texture
	auto moon_texture = bonobo::loadTexture2D("moon_diffuse.png");
	moonModel.set_geometry(sphere);
	moonModel.set_program(shader, [](GLuint /*program*/) {});
	moonModel.add_texture("diffuse_texture", moon_texture, GL_TEXTURE_2D);

	const auto moonStartRotation = random();
	const auto moonRotationScale = 1.0f / 10.0f;

	moonTilt.add_child(&moonModel);
#pragma endregion


	moon.add_child(&moonTilt);
#pragma endregion


	earth.add_child(&moon);
#pragma endregion


	glEnable(GL_DEPTH_TEST);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	double timeTemp;
	glm::vec3 vecTemp;

	// If camera is following the earth
	bool follow = false;

	// If camera is looking to the earth
	bool look = false;

	// Camera Distance From Earth
	float cameraDistance = 3.0f;

	// Delta Movement Camera From Earth
	float cameraDeltaMov = 0.1f;


	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		// Follow the earth
		if (inputHandler->GetKeycodeState(GLFW_KEY_F) & JUST_PRESSED)
		{
			follow = !follow;
		}

		// Look at the earth
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED)
		{
			look = !look;
		}

		// Get Closer to Earth
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & PRESSED)
		{
			cameraDistance -= cameraDeltaMov;
		}

		// Get Away from Earth
		if (inputHandler->GetKeycodeState(GLFW_KEY_X) & PRESSED)
		{
			cameraDistance += cameraDeltaMov;
		}

		ImGui_ImplGlfwGL3_NewFrame();


		//
		// How-To: Translate the sun
		//
		//sun.set_translation(glm::vec3(std::sin(nowTime), 0.0f, 0.0f));

		// Orbit Rotation

		// Earth
		timeTemp = earthStartOrbitRotation + nowTime * earthOrbitFrecuency;
		vecTemp = glm::vec3(std::sin(timeTemp), 0.0f, std::cos(timeTemp)) * earthSunDistanceRatio;
		earth.set_translation(vecTemp);

		// Follow the earth
		if (follow)
		{
			glm::vec3 dir2Cam = mCamera.mWorld.GetTranslation() - vecTemp;
			float distance2Cam = magnitude(dir2Cam);
			if (distance2Cam > cameraDistance)
			{
				mCamera.mWorld.SetTranslate(vecTemp + dir2Cam * cameraDistance / distance2Cam);
			}
		}

		// Look at the earth
		if (look)
		{
			mCamera.mWorld.LookAt(vecTemp);
		}

		// Moon
		timeTemp = moonStartOrbitRotation + nowTime * moonOrbitFrecuency;
		moonTilt.set_translation(glm::vec3(std::sin(timeTemp), 0.0f, std::cos(timeTemp)) * moonEarthDistance);


		// Tilt Rotation

		sunModel.set_rotation_y(sunStartRotation + nowTime * sunRotationScale);

		earthModel.set_rotation_y(earthStartRotation + nowTime * earthRotationScale);

		moonModel.set_rotation_y(moonStartRotation + nowTime * moonRotationScale);


		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Traverse the scene graph and render all the nodes
		auto node_stack = std::stack<Node const*>();
		auto matrix_stack = std::stack<glm::mat4>();
		node_stack.push(&world);
		matrix_stack.push(glm::mat4());
		do {
			auto const* const current_node = node_stack.top();
			node_stack.pop();

			auto const parent_matrix = matrix_stack.top();
			matrix_stack.pop();

			auto const current_node_matrix = current_node->get_transform();

			//
			// Todo: Compute the current node's world matrix
			//
			auto const current_node_world_matrix = parent_matrix * current_node_matrix;
			current_node->render(mCamera.GetWorldToClipMatrix(), current_node_world_matrix);

			for (int i = static_cast<int>(current_node->get_children_nb()) - 1; i >= 0; --i) {
				node_stack.push(current_node->get_child(static_cast<size_t>(i)));
				matrix_stack.push(current_node_world_matrix);
			}
		} while (!node_stack.empty());

		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(shader);
	shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment1 assignment1;
		assignment1.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
