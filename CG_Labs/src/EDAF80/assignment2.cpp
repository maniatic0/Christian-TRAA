#include "assignment2.hpp"
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

#include <array>
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

edaf80::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment2::~Assignment2()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment2::run()
{
	// Load the sphere geometry
	auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	if (shape.vao == 0u)
		return;

	// Load the quad geometry
	auto const shapeQuad = parametric_shapes::createQuad(5.0f, 10.0f);
	if (shapeQuad.vao == 0u)
		return;

	// Load the sphere geometry
	auto const shapeSphere = parametric_shapes::createSphere(60u, 60u, 0.5f);
	if (shapeSphere.vao == 0u)
		return;

	// Load the torus geometry
	auto const shapeTorus = parametric_shapes::createTorus(60u, 60u, 3.0f, 1.0f);
	if (shapeTorus.vao == 0u)
		return;

	// Load the mobius strip geometry
	auto const shapeStrip = parametric_shapes::createMobiusStrip(60u, 60u, 3.0f, 1.0f);
	if (shapeStrip.vao == 0u)
		return;

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
	if (diffuse_shader == 0u) {
		LogError("Failed to load diffuse shader");
		return;
	}
	auto normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
	if (normal_shader == 0u) {
		LogError("Failed to load normal shader");
		return;
	}
	auto texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
	if (texcoord_shader == 0u) {
		LogError("Failed to load texcoord shader");
		return;
	}

	auto size_shader = bonobo::createProgram("fallbackSize.vert", "fallback.frag");
	if (size_shader == 0u) {
		LogError("Failed to load size shader");
		return;
	}

	auto time = GetTimeSeconds();

	auto const light_position = glm::vec3(0.0f, 4.0f, 0.0f);
	auto const set_uniforms = [&light_position, &time](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform1f(glGetUniformLocation(program, "time"), time);
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.5f;

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension_2 = 0.5f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;

	auto circle_rings = Node();
	circle_rings.set_geometry(shape);
	circle_rings.set_program(fallback_shader, set_uniforms);

	auto quad = Node();
	quad.set_geometry(shapeQuad);
	quad.set_program(fallback_shader, set_uniforms);
	quad.set_translation(glm::vec3(5.0f, 0.0f, 5.0f));

	auto sphere = Node();
	sphere.set_geometry(shapeSphere);
	sphere.set_program(fallback_shader, set_uniforms);
	sphere.set_translation(glm::vec3(-2.0f, 0.0f, -2.0f));

	auto sphere2 = Node();
	sphere2.set_geometry(shapeSphere);
	sphere2.set_program(fallback_shader, set_uniforms);
	sphere2.set_translation(glm::vec3(-2.0f, 0.0f, -2.0f));

	auto torus = Node();
	torus.set_geometry(shapeTorus);
	torus.set_program(fallback_shader, set_uniforms);
	torus.set_translation(glm::vec3(10.0f, 10.0f, 10.0f));

	auto strip = Node();
	strip.set_geometry(shapeStrip);
	strip.set_program(fallback_shader, set_uniforms);
	strip.set_translation(glm::vec3(20.0f, 20.0f, 20.0f));


	//! \todo Create a tesselated sphere and a tesselated torus


	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
#define CULL_ASSIGNMENT_ENABLED // Enable Culling
#ifdef CULL_ASSIGNMENT_ENABLED
	glEnable(GL_CULL_FACE);
#define CULL_ASSIGNMENT_TYPE 1 // 1 for backculling
#if CULL_ASSIGNMENT_TYPE == 1
	glCullFace(GL_BACK);
#else
	glCullFace(GL_FRONT);
#endif // CULL_ASSIGNMENT_TYPE == 0
#endif // CULL_ASSIGNMENT_ENABLED

	// Interpolation
	auto const points = std::array<glm::vec3, 10>{
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 0.0f, 0.0f),
			glm::vec3(10.0f, 10.0f, 0.0f),
			glm::vec3(0.0f, 10.0f, 0.0f),
			glm::vec3(0.0f, 10.0f, 20.0f),
			glm::vec3(0.0f, 0.0f, 20.0f),
			glm::vec3(-10.0f, 0.0f, 20.0f),
			glm::vec3(-10.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 5.0f, 0.0f),
			glm::vec3(10.0f, 30.0f, 0.0f)
	};

	unsigned int step_number = 500, current_interpolation_index = 0;
	double interpolation_step = 1.0f / (static_cast<float>(step_number) - 1.0f);
	double current_interpolation_value = 0.0f;
	glm::vec3 interpolation_point;

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		time = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			circle_rings.set_program(fallback_shader, set_uniforms);
			quad.set_program(fallback_shader, set_uniforms);
			sphere.set_program(fallback_shader, set_uniforms);
			sphere2.set_program(fallback_shader, set_uniforms);
			torus.set_program(fallback_shader, set_uniforms);
			strip.set_program(fallback_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			circle_rings.set_program(diffuse_shader, set_uniforms);
			quad.set_program(diffuse_shader, set_uniforms);
			sphere.set_program(diffuse_shader, set_uniforms);
			sphere2.set_program(diffuse_shader, set_uniforms);
			torus.set_program(diffuse_shader, set_uniforms);
			strip.set_program(diffuse_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			circle_rings.set_program(normal_shader, set_uniforms);
			quad.set_program(normal_shader, set_uniforms);
			sphere.set_program(normal_shader, set_uniforms);
			sphere2.set_program(normal_shader, set_uniforms);
			torus.set_program(normal_shader, set_uniforms);
			strip.set_program(normal_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			circle_rings.set_program(texcoord_shader, set_uniforms);
			quad.set_program(texcoord_shader, set_uniforms);
			sphere.set_program(texcoord_shader, set_uniforms);
			sphere2.set_program(texcoord_shader, set_uniforms);
			torus.set_program(texcoord_shader, set_uniforms);
			strip.set_program(texcoord_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED) {
			circle_rings.set_program(size_shader, set_uniforms);
			quad.set_program(size_shader, set_uniforms);
			sphere.set_program(size_shader, set_uniforms);
			sphere2.set_program(size_shader, set_uniforms);
			torus.set_program(size_shader, set_uniforms);
			strip.set_program(size_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
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

		circle_rings.rotate_y(0.01f);
		sphere.rotate_y(0.01f);
		sphere.rotate_z(0.02f);
		quad.rotate_y(0.01f);
		torus.rotate_y(0.01f);
		torus.rotate_z(0.02f);
		strip.rotate_y(0.01f);


		//! \todo Interpolate the movement of a shape between various
		//!        control points

		current_interpolation_value += interpolation_step;
		if (current_interpolation_value > 1.0f)
		{
			current_interpolation_value = 0.0f;
			current_interpolation_index++;
			if (current_interpolation_index == points.size())
			{
				current_interpolation_index = 0;
			}
			
		}

		if (use_linear)
		{
			interpolation_point = interpolation::evalLERP(points[current_interpolation_index],
				points[current_interpolation_index + 1 == points.size() ? 0 : current_interpolation_index + 1],
				static_cast<float>(current_interpolation_value)
			);
			sphere.set_translation(interpolation_point);
			sphere2.set_translation(interpolation_point);
		}
		else
		{			
			sphere.set_translation(
				interpolation::evalCatmullRom(
					points[current_interpolation_index < 1 ? points.size() - 1 : current_interpolation_index - 1],
					points[current_interpolation_index],
					points[current_interpolation_index + 1 >= points.size() ? current_interpolation_index + 1 - points.size() : current_interpolation_index + 1],
					points[current_interpolation_index + 2 >= points.size() ? current_interpolation_index + 2 - points.size() : current_interpolation_index + 2],
					catmull_rom_tension,
					static_cast<float>(current_interpolation_value)
				)
			);
			sphere2.set_translation(
				interpolation::evalCatmullRom(
					points[current_interpolation_index < 1 ? points.size() - 1 : current_interpolation_index - 1],
					points[current_interpolation_index],
					points[current_interpolation_index + 1 >= points.size() ? current_interpolation_index + 1 - points.size() : current_interpolation_index + 1],
					points[current_interpolation_index + 2 >= points.size() ? current_interpolation_index + 2 - points.size() : current_interpolation_index + 2],
					catmull_rom_tension_2,
					static_cast<float>(current_interpolation_value)
				)
			);
		}

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		circle_rings.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());
		quad.render(mCamera.GetWorldToClipMatrix(), quad.get_transform());
		sphere.render(mCamera.GetWorldToClipMatrix(), sphere.get_transform());
		sphere2.render(mCamera.GetWorldToClipMatrix(), sphere2.get_transform());
		torus.render(mCamera.GetWorldToClipMatrix(), torus.get_transform());
		strip.render(mCamera.GetWorldToClipMatrix(), strip.get_transform());

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
			ImGui::SliderFloat("Catmull-Rom tension 2", &catmull_rom_tension_2, 0.0f, 1.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texcoord_shader);
	normal_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
