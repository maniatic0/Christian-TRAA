#include "assignment3.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment3::Assignment3()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 3", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment3::~Assignment3()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment3::run()
{
	// Load the Skybox sphere geometry
	auto skybox_sphere_shape = parametric_shapes::createSphere(100u, 100u, 100.0f);
	if (skybox_sphere_shape.vao == 0u) {
		LogError("Failed to retrieve the sphere mesh");
		return;
	}

	// Load the sphere geometry
	auto sphere_shape = parametric_shapes::createSphere(60u, 60u, 3.0f);
	if (sphere_shape.vao == 0u) {
		LogError("Failed to retrieve the sphere mesh");
		return;
	}

	// Light Position Sphere
	auto sphere_shape_light = parametric_shapes::createSphere(20u, 20u, 1.0f);
	if (sphere_shape_light.vao == 0u) {
		LogError("Failed to retrieve the sphere mesh");
		return;
	}


	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	GLuint diffuse_shader = 0u, normal_shader = 0u, texcoord_shader = 0u, phong_shader = 0u, blinn_phong_shader = 0u,
		skybox_shader = 0u;
	auto const reload_shaders = [&diffuse_shader,&normal_shader,&texcoord_shader,&phong_shader, &blinn_phong_shader, &skybox_shader](){
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

		if (phong_shader != 0u)
			glDeleteProgram(phong_shader);
		phong_shader = bonobo::createProgram("phong.vert", "phong.frag");
		if (phong_shader == 0u)
			LogError("Failed to load phong shader");

		
		if (blinn_phong_shader != 0u)
			glDeleteProgram(blinn_phong_shader);
		blinn_phong_shader = bonobo::createProgram("blinnPhong.vert", "blinnPhong.frag");
		if (blinn_phong_shader == 0u)
			LogError("Failed to load blinn-phong shader");

		if (skybox_shader != 0u)
			glDeleteProgram(skybox_shader);
		skybox_shader = bonobo::createProgram("skybox.vert", "skybox.frag");
		if (skybox_shader == 0u)
			LogError("Failed to load skybox shader");
	};
	reload_shaders();

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 1.0f;
	auto texture_information = 0;
	auto const phong_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess,&texture_information](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
		glUniform1i(glGetUniformLocation(program, "texture_information"), texture_information);
	};

	bool has_texture = false;
	bool has_cubemap = false;
	bool has_normal = false;

	auto polygon_mode = polygon_mode_t::fill;

	auto skybox_sphere = Node();
	skybox_sphere.set_geometry(skybox_sphere_shape);
	skybox_sphere.set_program(skybox_shader, set_uniforms);

	// Cubemap
	auto my_cube_map_1 = bonobo::loadTextureCubeMap("sunset_sky/posx.png", "sunset_sky/negx.png",
		"sunset_sky/posy.png", "sunset_sky/negy.png",
		"sunset_sky/posz.png", "sunset_sky/negz.png", true);
	skybox_sphere.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);

	auto sphere = Node();
	sphere.set_geometry(sphere_shape);
	sphere.set_program(fallback_shader, set_uniforms);
	auto sphere_texture = bonobo::loadTexture2D("fieldstone_diffuse.png");
	sphere.add_texture("diffuse_texture", sphere_texture, GL_TEXTURE_2D);
	auto sphere_bump_texture = bonobo::loadTexture2D("fieldstone_bump.png");
	sphere.add_texture("bump_texture", sphere_bump_texture, GL_TEXTURE_2D);
	sphere.add_texture("cube_map", my_cube_map_1, GL_TEXTURE_CUBE_MAP);

	auto sphere_light = Node();
	sphere_light.set_geometry(sphere_shape_light);
	sphere_light.set_program(fallback_shader, set_uniforms);

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
//#define CULL_ASSIGNMENT_ENABLED // Enable Culling
#ifdef CULL_ASSIGNMENT_ENABLED
	glEnable(GL_CULL_FACE);
#define CULL_ASSIGNMENT_TYPE 1 // 1 for backculling
#if CULL_ASSIGNMENT_TYPE == 1
	glCullFace(GL_BACK);
#else
	glCullFace(GL_FRONT);
#endif // CULL_ASSIGNMENT_TYPE == 0
#endif // CULL_ASSIGNMENT_ENABLED


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

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			sphere.set_program(fallback_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			sphere.set_program(diffuse_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			sphere.set_program(normal_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			sphere.set_program(texcoord_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED)
		{
			sphere.set_program(phong_shader, phong_set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_6) & JUST_PRESSED)
		{
			sphere.set_program(blinn_phong_shader, phong_set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}
		switch (polygon_mode) {
			case polygon_mode_t::fill:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case polygon_mode_t::line:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case polygon_mode_t::point:
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				break;
		}

		sphere_light.set_translation(light_position);
		camera_position = mCamera.mWorld.GetTranslation();

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		sphere.render(mCamera.GetWorldToClipMatrix(), sphere.get_transform());
		sphere_light.render(mCamera.GetWorldToClipMatrix(), sphere_light.get_transform());
		skybox_sphere.render(mCamera.GetWorldToClipMatrix(), skybox_sphere.get_transform());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
			ImGui::SliderFloat("Shininess", &shininess, 0.0f, 1000.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			ImGui::Checkbox("Has Texture?", &has_texture);
			if (has_texture)
			{
				texture_information = texture_information | 1;
			}
			else
			{
				texture_information = texture_information & ~1;
			}
			ImGui::Checkbox("Has Cube Map?", &has_cubemap);
			if (has_cubemap)
			{
				texture_information = texture_information | 1 << 1;
			}
			else
			{
				texture_information = texture_information & ~(1 << 1);
			}
			ImGui::Checkbox("Has Normal Map?", &has_normal);
			if (has_normal)
			{
				texture_information = texture_information | 1 << 2;
			}
			else
			{
				texture_information = texture_information & ~(1 << 2);
			}
			
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
		ImGui::End();

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texcoord_shader);
	texcoord_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
	glDeleteProgram(phong_shader);
	phong_shader = 0u;
	glDeleteProgram(blinn_phong_shader);
	blinn_phong_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment3 assignment3;
		assignment3.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
