#include "assignment4.hpp"
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

edaf80::Assignment4::Assignment4()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 4", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment4::~Assignment4()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment4::run()
{

	auto quad_shape = parametric_shapes::createQuad(700.0f, 700.0f, 50u, 50u);
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
	mCamera.mWorld.SetTranslate(glm::vec3(45.0f, 48.0f, 180.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	GLuint diffuse_shader = 0u, normal_shader = 0u, texcoord_shader = 0u, wave_shader = 0u, skybox_shader = 0u;
	auto const reload_shaders = [&diffuse_shader,&normal_shader,&texcoord_shader,&wave_shader,&skybox_shader](){
		if (diffuse_shader != 0u)
			glDeleteProgram(diffuse_shader);
		diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
		if (diffuse_shader == 0u)
			LogError("Failed to load diffuse shader");

		if (normal_shader != 0u)
			glDeleteProgram(normal_shader);
		normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
		if (normal_shader == 0u)
			LogError("Failed to load normal shader");

		if (texcoord_shader != 0u)
			glDeleteProgram(texcoord_shader);
		texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
		if (texcoord_shader == 0u)
			LogError("Failed to load texcoord shader");

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


	//
	// Todo: Load your geometry
	//

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};


	auto camera_position = mCamera.mWorld.GetTranslation();

	auto actual_time = 0.0f;

	auto amplitude1 = 1.0f;
	auto frequency1 = 0.2f;
	auto phase1 = 0.5f;
	auto sharpness1 = 2.0f;
	auto direction1 = glm::vec2(-1.0f,0.0f);

	auto amplitude2 = 0.5f;
	auto frequency2 = 0.4f;
	auto phase2 = 1.3f;
	auto sharpness2 = 2.0f;
	auto direction2 = glm::vec2(-0.7f,0.7f);
	auto color_deep = glm::vec4(0.0f, 0.0f, 0.1f, 1.0f);
	auto color_shallow = glm::vec4(0.0f, 0.5f, 0.5f, 1.0f);

	auto const wave_set_uniforms = [&light_position,&camera_position,&amplitude1, &frequency1, &phase1,
									&sharpness1, &direction1, &amplitude2, &frequency2, &phase2,
									&sharpness2,&direction2, &actual_time, &color_deep, &color_shallow](GLuint program){
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

	auto water_quad = Node();
	water_quad.set_geometry(quad_shape);
	water_quad.set_program(fallback_shader, set_uniforms);

	auto skybox_sphere = Node();
	skybox_sphere.set_geometry(skybox_sphere_shape);
	skybox_sphere.set_program(skybox_shader, set_uniforms);

	// Cubemap
	auto my_cube_map_1 = bonobo::loadTextureCubeMap("cloudyhills/posx.png", "cloudyhills/negx.png",
			"cloudyhills/posy.png", "cloudyhills/negy.png",
			"cloudyhills/posz.png", "cloudyhills/negz.png", true);
	water_quad.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);
	auto bump_texture = bonobo::loadTexture2D("waves.png");
	water_quad.add_texture("bump_texture", bump_texture, GL_TEXTURE_2D);
	skybox_sphere.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);


	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	f64 ddeltatime;
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
		mCamera.Update(ddeltatime, *inputHandler);

		camera_position = mCamera.mWorld.GetTranslation();

		ImGui_ImplGlfwGL3_NewFrame();

		//
		// Todo: If you need to handle inputs, you can do it here
		//

		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			water_quad.set_program(fallback_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			water_quad.set_program(diffuse_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			water_quad.set_program(normal_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			water_quad.set_program(texcoord_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED)
		{
			water_quad.set_program(wave_shader, wave_set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
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

		water_quad.render(mCamera.GetWorldToClipMatrix(), water_quad.get_transform());
		skybox_sphere.render(mCamera.GetWorldToClipMatrix(), skybox_sphere.get_transform());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();


		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		bool opened1 = ImGui::Begin("Wave 1", &opened1, ImVec2(300, 100), -1.0f, 0);
		if (opened1) {
			ImGui::SliderFloat("Amplitude 1", &amplitude1, 0.0f, 5.0f);
			ImGui::SliderFloat("Frequency 1", &frequency1, 0.0f, 1.0f);
			ImGui::SliderFloat("Phase 1", &phase1, 0.0f, 10.0f);
			ImGui::SliderFloat("Sharpness 1", &sharpness1, 0.0f, 10.0f);
			ImGui::SliderFloat2("Direction 1", glm::value_ptr(direction1), 0.0f, 10.0f);

		}
		ImGui::End();

		bool opened2 = ImGui::Begin("Wave 2", &opened2, ImVec2(300, 100), -1.0f, 0);
		if (opened2) {
			ImGui::SliderFloat("Amplitude 2", &amplitude2, 0.0f, 5.0f);
			ImGui::SliderFloat("Frequency 2", &frequency2, 0.0f, 1.0f);
			ImGui::SliderFloat("Phase 2", &phase2, 0.0f, 10.0f);
			ImGui::SliderFloat("Sharpness 2", &sharpness2, 0.0f, 10.0f);
			ImGui::SliderFloat2("Direction 2", glm::value_ptr(direction2), 0.0f, 10.0f);

		}
		ImGui::End();

		bool opened3 = ImGui::Begin("Scene Control", &opened3, ImVec2(300, 100), -1.0f, 0);
		if (opened2) {
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			ImGui::SliderFloat3("Camera Position", glm::value_ptr(camera_position), -200.0f, 200.0f);
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

	glDeleteProgram(wave_shader);
	wave_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment4 assignment4;
		assignment4.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
