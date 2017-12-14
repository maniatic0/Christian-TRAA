#include "assignment5.hpp"
#include "missile.h"
#include "plane.h"
#include "enemyPlane.h"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"
#include "core/node.hpp"


#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <stack>
#include <unordered_map>

edaf80::Assignment5::Assignment5()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 5", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment5::~Assignment5()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment5::run()
{

	auto quad_shape = parametric_shapes::createQuad(1400.0f, 1400.0f, 50u, 50u);
	if (quad_shape.vao == 0u) {
		LogError("Failed to retrieve the quad mesh");
		return;
	}

	// Load the Skybox sphere geometry
	auto skybox_sphere_shape = parametric_shapes::createSphere(100u, 100u, 700.0f);
	if (skybox_sphere_shape.vao == 0u) {
		LogError("Failed to retrieve the sphere mesh");
		return;
	}

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

#pragma region Shaders
	// Create the shader programs
	auto default_shader = bonobo::createProgram("default.vert", "default.frag");
	if (default_shader == 0u) {
		LogError("Failed to load default shader");
		return;
	}
	GLuint wave_shader = 0u, skybox_shader = 0u, phong_shader = 0u;
	auto const reload_shaders = [&wave_shader, &skybox_shader, &phong_shader]() {

		if (phong_shader != 0u)
			glDeleteProgram(phong_shader);
		phong_shader = bonobo::createProgram("texture_phong.vert", "texture_phong.frag");
		if (phong_shader == 0u)
			LogError("Failed to load phong shader");

		if (wave_shader != 0u)
			glDeleteProgram(wave_shader);
		wave_shader = bonobo::createProgram("wave.vert", "wave.frag");
		if (wave_shader == 0u)
			LogError("Failed to load wave shader");

		if (skybox_shader != 0u)
			glDeleteProgram(skybox_shader);
		skybox_shader = bonobo::createProgram("skybox.vert", "skybox.frag");
		if (skybox_shader == 0u)
			LogError("Failed to load skybox shader");

	};
	reload_shaders();


	auto camera_position = mCamera.mWorld.GetTranslation();
	auto light_position = glm::vec3(700.0f, 700.0f, 0.0f);
	auto ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 100.0f;
	auto texture_information = 1;
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &specular, &shininess, &texture_information](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
		glUniform1i(glGetUniformLocation(program, "texture_information"), texture_information);
	};

	auto const set_uniforms = [&light_position](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto actual_time = 0.0f;

	auto amplitude1 = 1.0f;
	auto frequency1 = 0.2f;
	auto phase1 = 0.5f;
	auto sharpness1 = 2.0f;
	auto direction1 = glm::vec2(1.0f, 0.0f);

	auto amplitude2 = 0.5f;
	auto frequency2 = 0.4f;
	auto phase2 = 1.3f;
	auto sharpness2 = 2.0f;
	auto direction2 = glm::vec2(1.0f, 0.7f);
	auto color_deep = glm::vec4(0.0f, 0.0f, 0.1f, 1.0f);
	auto color_shallow = glm::vec4(0.0f, 0.5f, 0.5f, 1.0f);

	auto const wave_set_uniforms = [&light_position, &camera_position, &amplitude1, &frequency1, &phase1,
		&sharpness1, &direction1, &amplitude2, &frequency2, &phase2,
		&sharpness2, &direction2, &actual_time, &color_deep, &color_shallow](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));

		glUniform1f(glGetUniformLocation(program, "amplitude1"), amplitude1);
		glUniform1f(glGetUniformLocation(program, "frequency1"), frequency1);
		glUniform1f(glGetUniformLocation(program, "phase1"), phase1);
		glUniform1f(glGetUniformLocation(program, "sharpness1"), sharpness1);
		glUniform2fv(glGetUniformLocation(program, "direction1"), 1, glm::value_ptr(direction1));

		glUniform1f(glGetUniformLocation(program, "amplitude2"), amplitude2);
		glUniform1f(glGetUniformLocation(program, "frequency2"), frequency2);
		glUniform1f(glGetUniformLocation(program, "phase2"), phase2);
		glUniform1f(glGetUniformLocation(program, "sharpness2"), sharpness2);
		glUniform2fv(glGetUniformLocation(program, "direction2"), 1, glm::value_ptr(direction2));

		glUniform1f(glGetUniformLocation(program, "actual_time"), actual_time);
		glUniform4fv(glGetUniformLocation(program, "color_deep"), 1, glm::value_ptr(color_deep));
		glUniform4fv(glGetUniformLocation(program, "color_shallow"), 1, glm::value_ptr(color_shallow));

	};

#pragma endregion


	//
	// Todo: Load your geometry
	//
	auto world = Node();

	auto missileGroup = Node();

	auto enemiesGroup = Node();

	auto plane = Plane(&mCamera, phong_shader, phong_set_uniforms);
	plane.set_translation(EnemyPlane::center);

	world.add_child(&missileGroup);
	world.add_child(&plane);
	world.add_child(&enemiesGroup);

	Missile::preloadMissiles(&missileGroup, 256, phong_shader, phong_set_uniforms);
	EnemyPlane::preloadPlanes(&enemiesGroup, &plane, 50, phong_shader, phong_set_uniforms);

	for (size_t i = 0; i < EnemyPlane::preloadedEnemyPlanes.size(); i++)
	{
		plane.enemies.emplace(static_cast<RigidBody*>(EnemyPlane::preloadedEnemyPlanes[i]));
	}

	mCamera.mWorld.SetTranslate(EnemyPlane::center - glm::vec3(0.0f, 0.0f, 200.0f));
	mCamera.mWorld.LookAt(EnemyPlane::center);

	std::unordered_map<const Node*, RigidBody*> rigidbodies;
	std::unordered_map<const Node*, RigidBody*>::const_iterator search_rigidbody;

	auto rigidList = RigidBody::getRigidBodyList();
	
	// Load al rigid bodies
	for (auto rigidIterator = rigidList->begin(); rigidIterator != rigidList->end(); rigidIterator++)
	{
		rigidbodies.emplace(static_cast<const Node*>(*rigidIterator), static_cast<RigidBody*>(*rigidIterator));
	}

	auto water_quad = Node();
	water_quad.set_geometry(quad_shape);
	water_quad.set_program(wave_shader, wave_set_uniforms);
	water_quad.translate(glm::vec3(700.0f, 0.0f, 700.0f));
	water_quad.rotate_y(bonobo::pi);

	auto skybox_sphere = Node();
	skybox_sphere.set_geometry(skybox_sphere_shape);
	skybox_sphere.set_program(skybox_shader, set_uniforms);

	//Add childs to a world
	world.add_child(&water_quad);
	world.add_child(&skybox_sphere);

	// Cubemap
	auto my_cube_map_1 = bonobo::loadTextureCubeMap("opensea/posx.png", "opensea/negx.png",
			"opensea/posy.png", "opensea/negy.png",
			"opensea/posz.png", "opensea/negz.png", true);
	water_quad.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);
	auto bump_texture = bonobo::loadTexture2D("waves.png");
	water_quad.add_texture("bump_texture", bump_texture, GL_TEXTURE_2D);
	skybox_sphere.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	// Physics Step Function
	auto const physicsStep = [&search_rigidbody, &world, &rigidbodies](float dt) {
		// Traverse the scene graph and render all the nodes
		auto node_stack = std::stack<Node const*>();
		auto matrix_stack = std::stack<glm::mat4>();
		node_stack.push(&world);
		matrix_stack.push(glm::mat4());
		do {
			Node const* current_node = node_stack.top();
			node_stack.pop();

			auto const parent_matrix = matrix_stack.top();
			matrix_stack.pop();

			if (!current_node->active)
			{
				continue;
			}

			search_rigidbody = rigidbodies.find(current_node);

			glm::mat4 current_node_world_matrix;

			if (search_rigidbody == rigidbodies.end())
			{
				auto const current_node_matrix = current_node->get_transform();
				current_node_world_matrix = parent_matrix * current_node->get_transform();
			}
			else
			{
				current_node_world_matrix = search_rigidbody->second->Update(parent_matrix, dt);
			}

			for (int i = static_cast<int>(current_node->get_children_nb()) - 1; i >= 0; --i) {
				node_stack.push(current_node->get_child(static_cast<size_t>(i)));
				matrix_stack.push(current_node_world_matrix);
			}
		} while (!node_stack.empty());
	};
	// Initialize Rigidbodies Positions
	physicsStep(0.0f);

	// Render Step Function
	auto const renderStep = [&world, &mCamera]() {
		// Traverse the scene graph and render all the nodes
		auto node_stack = std::stack<Node const*>();
		auto matrix_stack = std::stack<glm::mat4>();
		node_stack.push(&world);
		matrix_stack.push(glm::mat4());
		do {
			Node const* current_node = node_stack.top();
			node_stack.pop();

			auto const parent_matrix = matrix_stack.top();
			matrix_stack.pop();

			if (!current_node->active)
			{
				continue;
			}

			glm::mat4 current_node_world_matrix;

			auto const current_node_matrix = current_node->get_transform();
			current_node_world_matrix = parent_matrix * current_node->get_transform();
			current_node->render(mCamera.GetWorldToClipMatrix(), current_node_world_matrix);

			for (int i = static_cast<int>(current_node->get_children_nb()) - 1; i >= 0; --i) {
				node_stack.push(current_node->get_child(static_cast<size_t>(i)));
				matrix_stack.push(current_node_world_matrix);
			}
		} while (!node_stack.empty());
	};

	f64 ddeltatime;
	f64 dt;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		//mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		//
		// Todo: If you need to handle inputs, you can do it here
		//
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			EnemyPlane::deactivateEverything();
			Missile::deactivateEverything();
			plane.Revive();
			plane.set_translation(EnemyPlane::center);
			reload_shaders();
		}

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//
		// Todo: Render all your geometry here.
		//
		dt = ddeltatime / 1000.0f;

		skybox_sphere.rotate_y(dt);

		Node::UpdateNodes(inputHandler, dt);

		physicsStep(dt);

		RigidBody::checkCollisions();

		camera_position = mCamera.mWorld.GetTranslation();
		renderStep();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();


		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		bool opened1 = ImGui::Begin("Game Info", &opened1, ImVec2(300, 100), -1.0f, 0);
		if (opened1) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Score: %d", plane.score);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Time: %.2f", plane.time);
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Life: %d", plane.life);
		}
		ImGui::End();

		ImGui::Render();

		window->Swap();
		actual_time += ddeltatime / 1000.0f;
		lastTime = nowTime;
	}

	//
	// Todo: Do not forget to delete your shader programs, by calling
	//       `glDeleteProgram($your_shader_program)` for each of them.
	//
	glDeleteProgram(default_shader);
	default_shader = 0u;
	glDeleteProgram(phong_shader);
	phong_shader = 0u;
	glDeleteProgram(wave_shader);
	wave_shader = 0u;
	glDeleteProgram(skybox_shader);
	skybox_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment5 assignment5;
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
