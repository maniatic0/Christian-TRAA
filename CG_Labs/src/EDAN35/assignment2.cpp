#include "assignment2.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/GLStateInspection.h"
#include "core/GLStateInspectionView.h"
#include "core/helpers.hpp"
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

namespace constant
{
	constexpr uint32_t shadowmap_res_x = 1024;
	constexpr uint32_t shadowmap_res_y = 1024;

	constexpr size_t lights_nb           = 4; // 4
	constexpr float  light_intensity     = 720000.0f;
	constexpr float  light_angle_falloff = glm::radians(37.0f);
	constexpr float  light_cutoff        = 0.05f;
}

static bonobo::mesh_data loadCone();

edan35::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAN35: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);

	GLStateInspection::Init();
	GLStateInspection::View::Init();

	bonobo::init();
}

edan35::Assignment2::~Assignment2()
{
	bonobo::deinit();

	GLStateInspection::View::Destroy();
	GLStateInspection::Destroy();

	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edan35::Assignment2::run()
{
	// Load the geometry of Sponza
	auto const sponza_geometry = bonobo::loadObjects("../crysponza/sponza.obj");
	if (sponza_geometry.empty()) {
		LogError("Failed to load the Sponza model");
		return;
	}
	std::vector<Node> sponza_elements;
	sponza_elements.reserve(sponza_geometry.size() + 2);
	for (auto const& shape : sponza_geometry) {
		Node node;
		node.set_geometry(shape);
		sponza_elements.push_back(node);
	}

	// Load the sphere geometry
	auto const sphere_file = bonobo::loadObjects("sphere.obj");
	if (sphere_file.empty())
		return;
	auto const& sphere_geometry = sphere_file.front();

	// Sphere movement
	float amplitude = 300.0f;
	float frequency = 0.1f;
	auto is_sphere_paused = false;
	glm::vec3 sphere_pos(0.0f, 100.0f, 0.0f);

	Node sphere;
	sphere.set_geometry(sphere_geometry);
	sphere.set_scaling(glm::vec3(100.0f, 100.0f, 100.0f));
	sphere.set_translation(sphere_pos);
	sponza_elements.push_back(sphere);

	// Load the box geometry
	auto const box_file = bonobo::loadObjects("box.obj");
	if (box_file.empty())
		return;

	auto const& box_geometry = box_file.front();

	auto box_texture = bonobo::loadTexture2D("white.png");

	// Box movement
	glm::vec3 box_pos(2500.0f, 100.0f, 0.0f);
	float box_rotation = 0.25;

	Node box;
	box.set_geometry(box_geometry);
	box.set_scaling(glm::vec3(100.0f, 100.0f, 56.0f));
	box.set_rotation_z(bonobo::pi/2.0f);
	box.set_rotation_y(bonobo::two_pi * box_rotation);
	box.set_translation(box_pos);
	box.add_texture("diffuse_texture", box_texture, GL_TEXTURE_2D);
	sponza_elements.push_back(box);


	auto const cone_geometry = loadCone();
	Node cone;
	cone.set_geometry(cone_geometry);

	auto const window_size = window->GetDimensions();

	//
	// Setup the camera
	//
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(window_size.x) / static_cast<float>(window_size.y),
	                   1.0f, 10000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 100.0f, 180.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f, 0.0f, 0.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f;
	window->SetCamera(&mCamera);

	//
	// Load all the shader programs used
	//
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto const reload_shader = [fallback_shader](std::string const& vertex_path, std::string const& fragment_path, GLuint& program){
		if (program != 0u && program != fallback_shader)
			glDeleteProgram(program);
		program = bonobo::createProgram("../EDAN35/" + vertex_path, "../EDAN35/" + fragment_path);
		if (program == 0u) {
			LogError("Failed to load \"%s\" and \"%s\"", vertex_path.c_str(), fragment_path.c_str());
			program = fallback_shader;
		}
	};
	GLuint fill_gbuffer_shader = 0u, fill_shadowmap_shader = 0u,
		accumulate_lights_shader = 0u, resolve_deferred_shader = 0u,
		temporal_shader = 0u, resolve_temporal_shader = 0u,
		sharpen_shader = 0u, accumulation_shader = 0u,
		resolve_accumulation_shader = 0u;
	auto const reload_shaders = [&reload_shader,&fill_gbuffer_shader,
		&fill_shadowmap_shader,&accumulate_lights_shader,
		&resolve_deferred_shader,&temporal_shader,
		&resolve_temporal_shader, &sharpen_shader,
		&accumulation_shader, &resolve_accumulation_shader](){
		LogInfo("Reloading shaders");
		reload_shader("fill_gbuffer.vert",      "fill_gbuffer.frag",      fill_gbuffer_shader);
		reload_shader("fill_shadowmap.vert",    "fill_shadowmap.frag",    fill_shadowmap_shader);
		reload_shader("accumulate_lights.vert", "accumulate_lights.frag", accumulate_lights_shader);
		reload_shader("resolve_deferred.vert",  "resolve_deferred.frag",  resolve_deferred_shader);
		reload_shader("temporal.vert", "temporal.frag", temporal_shader);
		reload_shader("resolve_temporal.vert", "resolve_temporal.frag", resolve_temporal_shader);
		reload_shader("sharpen.vert", "sharpen.frag", sharpen_shader);
		reload_shader("accumulation.vert", "accumulation.frag", accumulation_shader);
		reload_shader("resolve_accumulation.vert", "resolve_accumulation.frag", resolve_accumulation_shader);
	};
	reload_shaders();

	auto const set_uniforms = [&mCamera](GLuint program){
		glUniformMatrix4fv(glGetUniformLocation(program, "unjittered_vertex_world_to_clip"), 1, GL_FALSE,
			glm::value_ptr(mCamera.GetWorldToClipMatrixUnjittered()));
	};


	//
	// Setup textures
	//
	auto const diffuse_texture                     = bonobo::createTexture(window_size.x, window_size.y);
	auto const specular_texture                    = bonobo::createTexture(window_size.x, window_size.y);
	auto const normal_texture                      = bonobo::createTexture(window_size.x, window_size.y);
	auto const light_diffuse_contribution_texture  = bonobo::createTexture(window_size.x, window_size.y);
	auto const light_specular_contribution_texture = bonobo::createTexture(window_size.x, window_size.y);
	auto const depth_texture                       = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	auto const shadowmap_texture                   = bonobo::createTexture(constant::shadowmap_res_x, constant::shadowmap_res_y, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);

	auto const deferred_shading_texture = bonobo::createTexture(window_size.x, window_size.y);
	GLuint const history_texture[] = { bonobo::createTexture(window_size.x, window_size.y), bonobo::createTexture(window_size.x, window_size.y) };
	auto const velocity_texture = bonobo::createTexture(window_size.x, window_size.y,GL_TEXTURE_2D, GL_RG16F);
	auto const temporal_output_texture = bonobo::createTexture(window_size.x, window_size.y);
	auto const sharpen_texture = bonobo::createTexture(window_size.x, window_size.y);
	auto const accumulation_texture = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT);

	//
	// Setup FBOs
	//
	auto const deferred_fbo  = bonobo::createFBO({diffuse_texture, specular_texture, normal_texture, velocity_texture}, depth_texture);
	auto const shadowmap_fbo = bonobo::createFBO({}, shadowmap_texture);
	auto const light_fbo     = bonobo::createFBO({light_diffuse_contribution_texture, light_specular_contribution_texture}, depth_texture);

	GLuint const history_fbo[] = { bonobo::createFBO({ history_texture[0], temporal_output_texture}, 0u), bonobo::createFBO({ history_texture[1], temporal_output_texture}, 0u) };
	auto const deferred_shading_fbo = bonobo::createFBO({ deferred_shading_texture }, 0u);
	auto const sharpen_fbo = bonobo::createFBO({ sharpen_texture }, 0u);
	auto const accumulation_fbo = bonobo::createFBO({ accumulation_texture }, 0u);

	//
	// Setup samplers
	//
	auto const default_sampler = bonobo::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	});
	auto const depth_sampler = bonobo::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	});
	auto const shadow_sampler = bonobo::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
		GLfloat border_color[4] = { 1.0f, 0.0f, 0.0f, 0.0f};
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border_color);
	});
	auto const bind_texture_with_sampler = [](GLenum target, unsigned int slot, GLuint program, std::string const& name, GLuint texture, GLuint sampler){
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, texture);
		glUniform1i(glGetUniformLocation(program, name.c_str()), static_cast<GLint>(slot));
		glBindSampler(slot, sampler);
	};


	//
	// Setup lights properties
	//
	std::array<TRSTransform<float, glm::defaultp>, constant::lights_nb + 1> lightTransforms;
	std::array<glm::vec3, constant::lights_nb + 1> lightColors;
	int lights_nb = static_cast<int>(constant::lights_nb) + 1;
	bool are_lights_paused = false;

	for (size_t i = 0; i < static_cast<int>(constant::lights_nb); ++i) {
		lightTransforms[i].SetTranslate(glm::vec3(0.0f, 125.0f, 0.0f));
		lightColors[i] = glm::vec3(0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		                           0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		                           0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));
	}

	lightTransforms[constant::lights_nb].SetTranslate(glm::vec3(0.0f, 1000.0f, 0.0f));
	lightTransforms[constant::lights_nb].SetRotateX(-bonobo::pi * 0.25f);
	lightColors[constant::lights_nb] = glm::vec3(0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		                        0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		                        0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));

	TRSTransform<f32, glm::defaultp> coneScaleTransform = TRSTransform<f32, glm::defaultp>();
	coneScaleTransform.SetScale(glm::vec3(sqrt(constant::light_intensity / constant::light_cutoff)));

	TRSTransform<f32, glm::defaultp> lightOffsetTransform = TRSTransform<f32, glm::defaultp>();
	lightOffsetTransform.SetTranslate(glm::vec3(0.0f, 0.0f, -40.0f));

	auto lightProjection = glm::perspective(bonobo::pi * 0.5f,
	                                        static_cast<float>(constant::shadowmap_res_x) / static_cast<float>(constant::shadowmap_res_y),
	                                        1.0f, 10000.0f);

	auto seconds_nb = 0.0f;
	auto seconds_sphere = 0.0f;

	// Save Vars
	bool save = false, save_steps = false;
	int samples = 16, current_samples = 0;
	const int FILE_NAME_SIZE = 200;
	char filename[FILE_NAME_SIZE+10] = "test";


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// TRAA variables
	glm::mat4 currentJitter;
	glm::vec2 windowInverseResolution;
	int history_switch = 0; // Where do we write and where do we read
	float k_feedback_max = 0.979f;
	float k_feedback_min = 0.925f;

	// Load old MVPS
	for (auto& element : sponza_elements) {
		element.oldMVP = mCamera.GetWorldToClipMatrixUnjittered() * element.get_transform();
	}

	double ddeltatime;
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
		if (!are_lights_paused)
			seconds_nb += static_cast<float>(ddeltatime / 1000.0);
		if(!is_sphere_paused)
			seconds_sphere += static_cast<float>(ddeltatime / 1000.0);

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

		// Sphere novement
		// The sphere is the penultimate element in vector
		sponza_elements[sponza_elements.size() - 2].set_translation(sphere_pos + glm::vec3(amplitude * std::sin(bonobo::two_pi * frequency * seconds_sphere), 0.0f, 0.0f));
		sponza_elements[sponza_elements.size() - 1].set_rotation_y(bonobo::two_pi * box_rotation);

		// TRAA vars
		windowInverseResolution.x = 1.0f / static_cast<float>(window_size.x);
		windowInverseResolution.y = 1.0f / static_cast<float>(window_size.y);
		currentJitter = mCamera.UpdateProjection(windowInverseResolution);


		auto const Deferred_Shading = [&deferred_fbo, &window_size,
			&sponza_elements, &mCamera, &fill_gbuffer_shader, &set_uniforms,
			&light_fbo, &lights_nb, &lightTransforms, &seconds_nb,
			&lightProjection, &lightOffsetTransform, &shadowmap_fbo,
			&fill_shadowmap_shader, &accumulate_lights_shader,
			&windowInverseResolution, &lightColors, &bind_texture_with_sampler,
			&depth_texture, &depth_sampler, &normal_texture, &default_sampler,
			&shadowmap_texture, &shadow_sampler, &coneScaleTransform,
			&cone, &deferred_shading_fbo, &resolve_deferred_shader,
			&diffuse_texture, &specular_texture,
			&light_diffuse_contribution_texture,
			&light_specular_contribution_texture]() {
			glDepthFunc(GL_LESS);
			//
			// Pass 1: Render scene into the g-buffer
			//
			glBindFramebuffer(GL_FRAMEBUFFER, deferred_fbo);
			GLenum const deferred_draw_buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, deferred_draw_buffers);
			auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				LogError("Something went wrong with framebuffer %u", deferred_fbo);
			glViewport(0, 0, window_size.x, window_size.y);
			glClearDepth(1.0f);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			// XXX: Is any other clearing needed?

			GLStateInspection::CaptureSnapshot("Filling Pass");

			for (auto& element : sponza_elements) {
				element.render(mCamera.GetWorldToClipMatrix(), element.get_transform(), fill_gbuffer_shader, set_uniforms);
				element.oldMVP = mCamera.GetWorldToClipMatrixUnjittered() * element.get_transform();
			}


			glCullFace(GL_FRONT);
			//
			// Pass 2: Generate shadowmaps and accumulate lights' contribution
			//
			glBindFramebuffer(GL_FRAMEBUFFER, light_fbo);
			GLenum light_draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, light_draw_buffers);
			glViewport(0, 0, window_size.x, window_size.y);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			// XXX: Is any clearing needed?
			for (size_t i = 0; i < static_cast<size_t>(lights_nb); ++i) {
				auto& lightTransform = lightTransforms[i];
				if (i != constant::lights_nb)
				{
					lightTransform.SetRotate(seconds_nb * 0.1f + i * 1.57f, glm::vec3(0.0f, 1.0f, 0.0f));
				}

				auto light_matrix = lightProjection * lightOffsetTransform.GetMatrixInverse() * lightTransform.GetMatrixInverse();

				//
				// Pass 2.1: Generate shadow map for light i
				//
				glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo);
				glViewport(0, 0, constant::shadowmap_res_x, constant::shadowmap_res_y);
				// XXX: Is any clearing needed?
				glClearDepth(1.0f);
				glClear(GL_DEPTH_BUFFER_BIT);

				GLStateInspection::CaptureSnapshot("Shadow Map Generation");

				for (auto const& element : sponza_elements) {
					element.render(light_matrix, glm::mat4(), fill_shadowmap_shader, set_uniforms);
				}



				glEnable(GL_BLEND);
				glDepthFunc(GL_GREATER);
				glDepthMask(GL_FALSE);
				glBlendEquationSeparate(GL_FUNC_ADD, GL_MIN);
				glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
				//
				// Pass 2.2: Accumulate light i contribution
				glBindFramebuffer(GL_FRAMEBUFFER, light_fbo);
				glDrawBuffers(2, light_draw_buffers);
				glUseProgram(accumulate_lights_shader);
				glViewport(0, 0, window_size.x, window_size.y);
				// XXX: Is any clearing needed?

				auto const spotlight_set_uniforms = [&windowInverseResolution, &mCamera, &light_matrix, &lightColors, &lightTransform, &i](GLuint program) {
					glUniform2f(glGetUniformLocation(program, "inv_res"),
						static_cast<float>(windowInverseResolution.x),
						static_cast<float>(windowInverseResolution.y));
					glUniformMatrix4fv(glGetUniformLocation(program, "view_projection_inverse"), 1, GL_FALSE,
						glm::value_ptr(mCamera.GetClipToWorldMatrix()));
					glUniform3fv(glGetUniformLocation(program, "camera_position"), 1,
						glm::value_ptr(mCamera.mWorld.GetTranslation()));
					glUniformMatrix4fv(glGetUniformLocation(program, "shadow_view_projection"), 1, GL_FALSE,
						glm::value_ptr(light_matrix));
					glUniform3fv(glGetUniformLocation(program, "light_color"), 1, glm::value_ptr(lightColors[i]));
					glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(lightTransform.GetTranslation()));
					glUniform3fv(glGetUniformLocation(program, "light_direction"), 1, glm::value_ptr(lightTransform.GetFront()));
					glUniform1f(glGetUniformLocation(program, "light_intensity"), constant::light_intensity);
					glUniform1f(glGetUniformLocation(program, "light_angle_falloff"), constant::light_angle_falloff);
					glUniform2f(glGetUniformLocation(program, "shadowmap_texel_size"),
						1.0f / static_cast<float>(constant::shadowmap_res_x),
						1.0f / static_cast<float>(constant::shadowmap_res_y));
				};

				bind_texture_with_sampler(GL_TEXTURE_2D, 0, accumulate_lights_shader, "depth_texture", depth_texture, depth_sampler);
				bind_texture_with_sampler(GL_TEXTURE_2D, 1, accumulate_lights_shader, "normal_texture", normal_texture, default_sampler);
				bind_texture_with_sampler(GL_TEXTURE_2D, 2, accumulate_lights_shader, "shadow_texture", shadowmap_texture, shadow_sampler);

				GLStateInspection::CaptureSnapshot("Accumulating");

				cone.render(mCamera.GetWorldToClipMatrix(),
					lightTransform.GetMatrix() * lightOffsetTransform.GetMatrix() * coneScaleTransform.GetMatrix(),
					accumulate_lights_shader, spotlight_set_uniforms);

				glBindSampler(2u, 0u);
				glBindSampler(1u, 0u);
				glBindSampler(0u, 0u);

				glDepthMask(GL_TRUE);
				glDepthFunc(GL_LESS);
				glDisable(GL_BLEND);
			}


			glCullFace(GL_BACK);
			glDepthFunc(GL_ALWAYS);
			//
			// Pass 3: Compute final image using both the g-buffer and  the light accumulation buffer
			//
			glBindFramebuffer(GL_FRAMEBUFFER, deferred_shading_fbo);
			GLenum const resolve_deferred_texture_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, resolve_deferred_texture_draw_buffers);
			auto const status_resolve_deferred_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status_resolve_deferred_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
				LogError("Something went wrong with framebuffer %u", deferred_shading_fbo);
			glUseProgram(resolve_deferred_shader);
			glViewport(0, 0, window_size.x, window_size.y);
			// XXX: Is any clearing needed?

			bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_deferred_shader, "diffuse_texture", diffuse_texture, default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 1, resolve_deferred_shader, "specular_texture", specular_texture, default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 2, resolve_deferred_shader, "light_d_texture", light_diffuse_contribution_texture, default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 3, resolve_deferred_shader, "light_s_texture", light_specular_contribution_texture, default_sampler);

			GLStateInspection::CaptureSnapshot("Resolve Pass");

			bonobo::drawFullscreen();

			glBindSampler(3, 0u);
			glBindSampler(2, 0u);
			glBindSampler(1, 0u);
			glBindSampler(0, 0u);
			glUseProgram(0u);
		};

		Deferred_Shading();

		//
		// Pass 4: Temporal Reprojection AA
		//
		glBindFramebuffer(GL_FRAMEBUFFER, history_fbo[(history_switch + 1) & 1]);
		GLenum const history_texture_draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		glDrawBuffers(2, history_texture_draw_buffers);
		auto const status_history_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_history_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", history_fbo);
		glUseProgram(temporal_shader);

		auto const temporal_set_uniforms = [&windowInverseResolution, &mCamera, &currentJitter,
			&k_feedback_max, &k_feedback_min] (GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
			glUniformMatrix4fv(glGetUniformLocation(program, "jitter"), 1, GL_FALSE,
				glm::value_ptr(currentJitter));
			glUniform1f(glGetUniformLocation(program, "k_feedback_max"), k_feedback_max);
			glUniform1f(glGetUniformLocation(program, "k_feedback_min"), k_feedback_min);
			glUniform1f(glGetUniformLocation(program, "z_near"), mCamera.mNear);
			glUniform1f(glGetUniformLocation(program, "z_far"), mCamera.mFar);
		};

		temporal_set_uniforms(temporal_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, temporal_shader, "history_texture", history_texture[history_switch & 1], default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, temporal_shader, "depth_texture", depth_texture, depth_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 2, temporal_shader, "velocity_texture", velocity_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 3, temporal_shader, "current_texture", deferred_shading_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 4, temporal_shader, "diffuse_texture", diffuse_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("Temporal Pass");

		bonobo::drawFullscreen();

		glBindSampler(4, 0u);
		glBindSampler(3, 0u);
		glBindSampler(2, 0u);
		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);

		//
		// Pass 4: Temporal Reprojection AA
		//
		glBindFramebuffer(GL_FRAMEBUFFER, sharpen_fbo);
		GLenum const sharpen_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, sharpen_draw_buffers);
		auto const status_sharpen_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_sharpen_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", sharpen_fbo);
		glUseProgram(sharpen_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		auto const sharpen_set_uniforms = [&windowInverseResolution](GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
		};

		sharpen_set_uniforms(sharpen_shader);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, temporal_shader, "temporal_output", temporal_output_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("Sharpen Pass");

		bonobo::drawFullscreen();

		glBindSampler(0, 0u);
		glUseProgram(0u);

		//
		// Pass 5: Resolve Temporal Reprojection AA
		//
		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
		glUseProgram(resolve_temporal_shader);
		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		auto const postprocessing_set_uniforms = [&windowInverseResolution, &ddeltatime, &currentJitter](GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
			glUniform1f(glGetUniformLocation(program, "current_fps"), ddeltatime);
			glUniformMatrix4fv(glGetUniformLocation(program, "jitter"), 1, GL_FALSE,
				glm::value_ptr(currentJitter));
		};

		postprocessing_set_uniforms(resolve_temporal_shader);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_temporal_shader, "temporal_output", sharpen_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, resolve_temporal_shader, "velocity_texture", velocity_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("Resolve Temporal Pass");

		bonobo::drawFullscreen();

		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);


		if (save)
		{
			if (current_samples < CAMERA_JITTERING_SIZE)
			{
				if (save_steps)
				{
					std::string file_name(filename);
					file_name += "_temporal_" + std::to_string(current_samples);
					bonobo::screenShot(file_name, static_cast<int>(window_size.x), static_cast<int>(window_size.y));
				}

				if (current_samples == CAMERA_JITTERING_SIZE - 1)
				{
					if (!save_steps)
					{
						std::string file_name(filename);
						file_name += "_temporal" ;
						bonobo::screenShot(file_name, static_cast<int>(window_size.x), static_cast<int>(window_size.y));
					}

					const float samples_inverse = 1.0 / static_cast<float>(samples);

					auto const accumulation_set_uniforms = [&samples_inverse](GLuint program) {
						glUniform1f(glGetUniformLocation(program, "samples_inverse"), samples_inverse);
					};

					for (size_t i = 0; i < samples; i++)
					{
						currentJitter = mCamera.UpdateProjection(windowInverseResolution);
						Deferred_Shading();

						glFinish();
						glEnable(GL_BLEND);
						glBlendFunc(GL_ONE, GL_ONE);

						//
						// Pass: Accumulation
						//
						glBindFramebuffer(GL_FRAMEBUFFER, accumulation_fbo);
						GLenum const accumulation_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
						glDrawBuffers(1, accumulation_draw_buffers);
						auto const status_accumulation_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
						if (status_accumulation_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
							LogError("Something went wrong with framebuffer %u", accumulation_fbo);
						glUseProgram(accumulation_shader);

						glViewport(0, 0, window_size.x, window_size.y);

						if (i == 0) // Clear the things from before
						{
							glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
							glClear(GL_COLOR_BUFFER_BIT);
						}
						

						accumulation_set_uniforms(accumulation_shader);

						bind_texture_with_sampler(GL_TEXTURE_2D, 0, accumulation_shader, "deferred_texture", deferred_shading_texture, default_sampler);

						GLStateInspection::CaptureSnapshot("Accumulation Pass");

						bonobo::drawFullscreen();

						glBindSampler(0, 0u);
						glUseProgram(0u);
						
						glDisable(GL_BLEND);
					}

					//
					// Pass: Accumulation Resolve
					//
					glFinish();
					glBindFramebuffer(GL_FRAMEBUFFER, 0u);
					glUseProgram(resolve_accumulation_shader);
					glViewport(0, 0, window_size.x, window_size.y);
					glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
					glClear(GL_COLOR_BUFFER_BIT);

					bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_accumulation_shader, "accumulation_texture", accumulation_texture, default_sampler);

					GLStateInspection::CaptureSnapshot("Accumulation Resolve Pass");

					bonobo::drawFullscreen();

					glBindSampler(0, 0u);
					glUseProgram(0u);


					std::string file_name(filename);
					file_name += "_ground_truth";
					bonobo::screenShot(file_name, static_cast<int>(window_size.x), static_cast<int>(window_size.y));
					save = false;
				}
			}
			current_samples++;
		}



		//
		// Pass 4: Draw wireframe cones on top of the final image for debugging purposes
		//
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//		for (size_t i = 0; i < static_cast<size_t>(lights_nb); ++i) {
//			cone.render(mCamera.GetWorldToClipMatrix(),
//			            lightTransforms[i].GetMatrix() * lightOffsetTransform.GetMatrix() * coneScaleTransform.GetMatrix(),
//			            fill_shadowmap_shader, set_uniforms);
//		}
//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


		//
		// Output content of the g-buffer as well as of the shadowmap, for debugging purposes
		//
		bonobo::displayTexture({-0.95f, -0.95f}, {-0.55f, -0.55f}, diffuse_texture,                     default_sampler, {0, 1, 2, -1}, window_size);
		bonobo::displayTexture({-0.45f, -0.95f}, {-0.05f, -0.55f}, specular_texture,                    default_sampler, {0, 1, 2, -1}, window_size);
		bonobo::displayTexture({ 0.05f, -0.95f}, { 0.45f, -0.55f}, normal_texture,                      default_sampler, {0, 1, 2, -1}, window_size);
		bonobo::displayTexture({ 0.55f, -0.95f}, { 0.95f, -0.55f}, depth_texture,                       default_sampler, {0, 0, 0, -1}, window_size, &mCamera);
		bonobo::displayTexture({-0.95f,  0.55f}, {-0.55f,  0.95f}, shadowmap_texture,                   default_sampler, {0, 0, 0, -1}, window_size, &mCamera);
		bonobo::displayTexture({-0.45f,  0.55f}, {-0.05f,  0.95f}, light_diffuse_contribution_texture,  default_sampler, {0, 1, 2, -1}, window_size);
		bonobo::displayTexture({ 0.05f,  0.55f}, { 0.45f,  0.95f}, light_specular_contribution_texture, default_sampler, {0, 1, 2, -1}, window_size);
		bonobo::displayTexture({ 0.55f,  0.55f }, { 0.95f,  0.95f }, velocity_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
		//bonobo::displayTexture({ -1.0f,  -1.0f }, { 1.0f,  1.0f }, accumulation_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
		//
		// Reset viewport back to normal
		//
		glViewport(0, 0, window_size.x, window_size.y);

		GLStateInspection::View::Render();
		Log::View::Render();

		bool opened = ImGui::Begin("Render Time", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
		ImGui::End();

		opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened) {
			ImGui::Checkbox("Jitter?", &mCamera.jitterProjection);
			ImGui::SliderFloat("Jitter Spread", &mCamera.jitterSpread, 0.0f, 1.0f);
			ImGui::SliderFloat("k_feedback_min", &k_feedback_min, 0.0f, 1.0f);
			ImGui::SliderFloat("k_feedback_max", &k_feedback_max, 0.0f, 1.0f);
			ImGui::Checkbox("Pause lights", &are_lights_paused);
			ImGui::SliderInt("Number of lights", &lights_nb, 1, static_cast<int>(constant::lights_nb)+1);
			ImGui::SliderFloat("Box Rotation", &box_rotation, 0.0f, 1.0f);
			ImGui::Checkbox("Pause sphere", &is_sphere_paused);
			ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 1000.0f);
			ImGui::SliderFloat("Frequency", &frequency, 0.0f, 10.0f);
			ImGui::SliderFloat3("Sphere Home Position", glm::value_ptr(sphere_pos), -2000.0f, 2000.0f);
		}
		ImGui::End();

		opened = ImGui::Begin("Save Image", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened && !save)
		{
			ImGui::InputText("Filename", filename, FILE_NAME_SIZE);
			ImGui::Checkbox("Save Steps", &save_steps);
			ImGui::SliderInt("Sample Amount", &samples, 1, 256);
			if (ImGui::Button("Save Image"))
			{
				save = true;
				current_samples = 0;
			}
		}
		ImGui::End();

		ImGui::Render();

		window->Swap();

		history_switch++;
		if (history_switch >= 2) history_switch -= 2;

		lastTime = nowTime;
	}

	glDeleteProgram(resolve_accumulation_shader);
	resolve_accumulation_shader = 0u;
	glDeleteProgram(accumulation_shader);
	accumulation_shader = 0u;
	glDeleteProgram(resolve_temporal_shader);
	resolve_temporal_shader = 0u;
	glDeleteProgram(sharpen_shader);
	sharpen_shader = 0u;
	glDeleteProgram(temporal_shader);
	temporal_shader = 0u;
	glDeleteProgram(resolve_deferred_shader);
	resolve_deferred_shader = 0u;
	glDeleteProgram(accumulate_lights_shader);
	accumulate_lights_shader = 0u;
	glDeleteProgram(fill_shadowmap_shader);
	fill_shadowmap_shader = 0u;
	glDeleteProgram(fill_gbuffer_shader);
	fill_gbuffer_shader = 0u;
	glDeleteProgram(fallback_shader);
	fallback_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edan35::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}

static
bonobo::mesh_data
loadCone()
{
	bonobo::mesh_data cone;
	cone.vertices_nb = 65;
	cone.drawing_mode = GL_TRIANGLE_STRIP;
	float vertexArrayData[65 * 3] = {
		0.f, 1.f, -1.f,
		0.f, 0.f, 0.f,
		0.38268f, 0.92388f, -1.f,
		0.f, 0.f, 0.f,
		0.70711f, 0.70711f, -1.f,
		0.f, 0.f, 0.f,
		0.92388f, 0.38268f, -1.f,
		0.f, 0.f, 0.f,
		1.f, 0.f, -1.f,
		0.f, 0.f, 0.f,
		0.92388f, -0.38268f, -1.f,
		0.f, 0.f, 0.f,
		0.70711f, -0.70711f, -1.f,
		0.f, 0.f, 0.f,
		0.38268f, -0.92388f, -1.f,
		0.f, 0.f, 0.f,
		0.f, -1.f, -1.f,
		0.f, 0.f, 0.f,
		-0.38268f, -0.92388f, -1.f,
		0.f, 0.f, 0.f,
		-0.70711f, -0.70711f, -1.f,
		0.f, 0.f, 0.f,
		-0.92388f, -0.38268f, -1.f,
		0.f, 0.f, 0.f,
		-1.f, 0.f, -1.f,
		0.f, 0.f, 0.f,
		-0.92388f, 0.38268f, -1.f,
		0.f, 0.f, 0.f,
		-0.70711f, 0.70711f, -1.f,
		0.f, 0.f, 0.f,
		-0.38268f, 0.92388f, -1.f,
		0.f, 1.f, -1.f,
		0.f, 1.f, -1.f,
		0.38268f, 0.92388f, -1.f,
		0.f, 1.f, -1.f,
		0.70711f, 0.70711f, -1.f,
		0.f, 0.f, -1.f,
		0.92388f, 0.38268f, -1.f,
		0.f, 0.f, -1.f,
		1.f, 0.f, -1.f,
		0.f, 0.f, -1.f,
		0.92388f, -0.38268f, -1.f,
		0.f, 0.f, -1.f,
		0.70711f, -0.70711f, -1.f,
		0.f, 0.f, -1.f,
		0.38268f, -0.92388f, -1.f,
		0.f, 0.f, -1.f,
		0.f, -1.f, -1.f,
		0.f, 0.f, -1.f,
		-0.38268f, -0.92388f, -1.f,
		0.f, 0.f, -1.f,
		-0.70711f, -0.70711f, -1.f,
		0.f, 0.f, -1.f,
		-0.92388f, -0.38268f, -1.f,
		0.f, 0.f, -1.f,
		-1.f, 0.f, -1.f,
		0.f, 0.f, -1.f,
		-0.92388f, 0.38268f, -1.f,
		0.f, 0.f, -1.f,
		-0.70711f, 0.70711f, -1.f,
		0.f, 0.f, -1.f,
		-0.38268f, 0.92388f, -1.f,
		0.f, 0.f, -1.f,
		0.f, 1.f, -1.f,
		0.f, 0.f, -1.f
	};

	glGenVertexArrays(1, &cone.vao);
	assert(cone.vao != 0u);
	glBindVertexArray(cone.vao);
	{
		glGenBuffers(1, &cone.bo);
		assert(cone.bo != 0u);
		glBindBuffer(GL_ARRAY_BUFFER, cone.bo);
		glBufferData(GL_ARRAY_BUFFER, cone.vertices_nb * 3 * sizeof(float), vertexArrayData, GL_STATIC_DRAW);

		glVertexAttribPointer(static_cast<int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));
		glEnableVertexAttribArray(static_cast<int>(bonobo::shader_bindings::vertices));

		glBindBuffer(GL_ARRAY_BUFFER, 0u);
	}
	glBindVertexArray(0u);

	return cone;
}
