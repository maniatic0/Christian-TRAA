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
#include <sstream>

// SMAA
#include "AreaTex.h"
#include "SearchTex.h"

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
	sponza_elements.reserve(sponza_geometry.size() + 10);
	for (auto const& shape : sponza_geometry) {
		Node node;
		node.set_geometry(shape);
		sponza_elements.push_back(node);
	}

	auto white_normal_texture = bonobo::loadTexture2D("../../testing/models/textures/white_normal.png");
	auto white_specular_texture = bonobo::loadTexture2D("../../testing/models/textures/white_specular.png");

	// Load pipe geometry
	auto const pipe_geometry = bonobo::loadObjects("SA_LD_Pipe.obj", true);
	if (pipe_geometry.empty()) {
		LogError("Failed to load the Pipe model, check the testing folder for the pipe");
		return;
	}
		
	const glm::vec3 pipe_pos(2300.0f, 100.0f, 0.0f);
	const glm::vec3 pipe_scale(60.0f, 60.0f, 60.0f);

	for (auto const& shape : pipe_geometry) {
		Node node;
		node.set_geometry(shape);
		node.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
		node.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
		node.set_translation(pipe_pos);
		node.set_scaling(pipe_scale);
		sponza_elements.push_back(node);
	}

	// Load Windows Arch geometry
	auto const window_arch_geometry = bonobo::loadObjects("window_arch.obj", true);
	if (window_arch_geometry.empty()) {
		LogError("Failed to load the Windows Arch model, check the testing folder for the Windows Arch");
		return;
	}

	const glm::vec3 window_arch_pos(2300.0f, 100.0f, 50.0f);
	const glm::vec3 window_arch_scale(1.0f, 1.0f, 1.0f);

	for (auto const& shape : window_arch_geometry) {
		Node node;
		node.set_geometry(shape);
		node.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
		node.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
		node.set_translation(window_arch_pos);
		node.set_scaling(window_arch_scale);
		sponza_elements.push_back(node);
	}

	// Load Windows Square geometry
	auto const window_square_geometry = bonobo::loadObjects("window_square.obj", true);
	if (window_square_geometry.empty()) {
		LogError("Failed to load the Windows Square model, check the testing folder for the Windows Square");
		return;
	}

	const glm::vec3 window_square_pos(2300.0f, 100.0f, -50.0f);
	const glm::vec3 window_square_scale(1.0f, 1.0f, 1.0f);

	for (auto const& shape : window_square_geometry) {
		Node node;
		node.set_geometry(shape);
		node.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
		node.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
		node.set_translation(window_square_pos);
		node.set_rotation_y(bonobo::pi / 2.0f);
		node.set_scaling(window_square_scale);
		sponza_elements.push_back(node);
	}

	// Load Hairball geometry
	auto const hairball_geometry = bonobo::loadObjects("hairball.obj", true);
	if (hairball_geometry.empty()) {
		LogError("Failed to load the Windows Square model, check the testing folder for the Windows Square");
		return;
	}

	float hairball_rotation_speed = 0.3f;
	auto is_hairball_paused = false;

	Node hairball;
	hairball.set_geometry(hairball_geometry.front());
	hairball.add_texture("diffuse_texture", white_normal_texture, GL_TEXTURE_2D);
	hairball.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
	hairball.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
	hairball.set_scaling(glm::vec3(10.0f, 10.0f, 10.0f));
	hairball.set_translation(glm::vec3(-900.0f, 100.0f, 500.0f));
	sponza_elements.push_back(hairball);

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
	sphere.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
	sphere.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
	sphere.set_geometry(sphere_geometry);
	sphere.set_scaling(glm::vec3(100.0f, 100.0f, 100.0f));
	sphere.set_translation(sphere_pos);
	sponza_elements.push_back(sphere);

	// Load the box geometry
	auto const box_file = bonobo::loadObjects("box.obj");
	if (box_file.empty())
		return;

	auto const& box_geometry = box_file.front();

	auto box_texture = bonobo::loadTexture2D("../../testing/models/textures/white.png");

	// Box movement
	glm::vec3 box_pos(2700.0f, 100.0f, 0.0f);
	float box_rotation = 0.25;

	Node box;
	box.set_geometry(box_geometry);
	box.set_translation(box_pos);
	box.set_scaling(glm::vec3(200.0f, 200.0f, 200.0f));
	box.set_rotation_z(bonobo::pi/2.0f);
	box.set_rotation_y(bonobo::two_pi * box_rotation);
	box.add_texture("diffuse_texture", box_texture, GL_TEXTURE_2D);
	box.add_texture("normals_texture", white_normal_texture, GL_TEXTURE_2D);
	box.add_texture("specular_texture", white_specular_texture, GL_TEXTURE_2D);
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
	auto const reload_shader = [fallback_shader](std::string const& vertex_path, std::string const& fragment_path, GLuint& program, std::string const& macros=""){
		if (program != 0u && program != fallback_shader)
			glDeleteProgram(program);
		program = bonobo::createProgram("../EDAN35/" + vertex_path, "../EDAN35/" + fragment_path, macros);
		if (program == 0u) {
			LogError("Failed to load \"%s\" and \"%s\"", vertex_path.c_str(), fragment_path.c_str());
			program = fallback_shader;
		}
	};
	GLuint fill_gbuffer_shader = 0u, fill_shadowmap_shader = 0u,
		accumulate_lights_shader = 0u, resolve_deferred_shader = 0u,
		temporal_shader = 0u, resolve_temporal_shader = 0u,
		sharpen_shader = 0u, accumulation_shader = 0u,
		resolve_accumulation_shader = 0u, resolve_no_aa_shader = 0u,
		sobel_shader = 0u, temporal_with_sobel_shader = 0u,
		temporal_for_Sobel_shader = 0u, fxaa_shader = 0u,
		clear_gbuffer_shader = 0u, uncharted_sharpen_shader = 0u,
		smaa_edge_shader = 0u, smaa_blend_weight_shader = 0u,
		smaa_blending_shader = 0u;
	auto const reload_shaders = [&reload_shader,&fill_gbuffer_shader,
		&fill_shadowmap_shader,&accumulate_lights_shader,
		&resolve_deferred_shader,&temporal_shader,
		&resolve_temporal_shader, &sharpen_shader,
		&accumulation_shader, &resolve_accumulation_shader,
		&resolve_no_aa_shader, &sobel_shader,
		&temporal_with_sobel_shader, &temporal_for_Sobel_shader,
		&fxaa_shader, &clear_gbuffer_shader,
		&uncharted_sharpen_shader, &smaa_edge_shader,
		&smaa_blend_weight_shader, &smaa_blending_shader](){
		LogInfo("Reloading shaders");
		reload_shader("fill_gbuffer.vert",      "fill_gbuffer.frag",      fill_gbuffer_shader);
		reload_shader("clear_gbuffer.vert", "clear_gbuffer.frag",  clear_gbuffer_shader);
		reload_shader("fill_shadowmap.vert",    "fill_shadowmap.frag",    fill_shadowmap_shader);
		reload_shader("accumulate_lights.vert", "accumulate_lights.frag", accumulate_lights_shader);
		reload_shader("resolve_deferred.vert",  "resolve_deferred.frag",  resolve_deferred_shader);
		reload_shader("temporal.vert", "temporal.frag", temporal_shader);
		reload_shader("resolve_temporal.vert", "resolve_temporal.frag", resolve_temporal_shader);
		reload_shader("sharpen.vert", "sharpen.frag", sharpen_shader);
		reload_shader("sharpen.vert", "sharpen.frag", uncharted_sharpen_shader, "#version 410\n#define USE_UNCHARTED_4_SATURATE");
		reload_shader("accumulation.vert", "accumulation.frag", accumulation_shader);
		reload_shader("resolve_accumulation.vert", "resolve_accumulation.frag", resolve_accumulation_shader);
		reload_shader("resolve_no_aa.vert", "resolve_no_aa.frag", resolve_no_aa_shader);
		reload_shader("sobel.vert", "sobel.frag", sobel_shader);
		reload_shader("temporal_with_sobel.vert", "temporal_with_sobel.frag", temporal_with_sobel_shader);
		reload_shader("sobel_temporal.vert","sobel_temporal.frag", temporal_for_Sobel_shader);
		reload_shader("fxaa.vert", "fxaa.frag", fxaa_shader);
		reload_shader("smaa_edge.vert", "smaa_edge.frag", smaa_edge_shader);
		reload_shader("smaa_blend_weight.vert", "smaa_blend_weight.frag", smaa_blend_weight_shader);
		reload_shader("smaa_blending.vert", "smaa_blending.frag", smaa_blending_shader);
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

	GLuint const history_texture[] = { bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT),
		bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT) };
	GLuint const history_improved_texture[] = { bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT),
		bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT) };

	GLuint const depth_history_texture[] = { bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R16F, GL_RED, GL_FLOAT),
		bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R16F, GL_RED, GL_FLOAT) };

	auto const velocity_texture = bonobo::createTexture(window_size.x, window_size.y,GL_TEXTURE_2D, GL_RG16F, GL_RG, GL_FLOAT);
	auto const temporal_output_texture = bonobo::createTexture(window_size.x, window_size.y);
	auto const sharpen_texture = bonobo::createTexture(window_size.x, window_size.y);
	
	auto const sobel_current_texture = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R32F, GL_RED, GL_FLOAT);
	GLuint const sobel_texture[] = { bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R32F, GL_RED, GL_FLOAT),
		bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R32F, GL_RED, GL_FLOAT) };

	auto const accumulation_texture = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT);

	auto const model_index_texture = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);

	//
	// Setup FBOs
	//
	auto const depth_clearing_fbo = bonobo::createFBO({ }, depth_texture);
	auto const clearing_fbo = bonobo::createFBO({ diffuse_texture, specular_texture, normal_texture, velocity_texture, model_index_texture}, 0u);
	auto const deferred_fbo  = bonobo::createFBO({diffuse_texture, specular_texture, normal_texture, velocity_texture, model_index_texture}, depth_texture);
	auto const shadowmap_fbo = bonobo::createFBO({}, shadowmap_texture);
	auto const light_fbo     = bonobo::createFBO({light_diffuse_contribution_texture, light_specular_contribution_texture}, depth_texture);

	auto const deferred_shading_fbo = bonobo::createFBO({ deferred_shading_texture }, 0u);

	auto const sobel_current_fbo = bonobo::createFBO({ sobel_current_texture }, 0u);
	GLuint const sobel_fbo[] = { bonobo::createFBO({ sobel_texture[0] }, 0u),  bonobo::createFBO({ sobel_texture[1] }, 0u) };

	GLuint const history_fbo[] = { bonobo::createFBO({ history_texture[0], temporal_output_texture, depth_history_texture[0] }, 0u),
		bonobo::createFBO({ history_texture[1], temporal_output_texture, depth_history_texture[1] }, 0u) };
	GLuint const history_improved_fbo[] = { bonobo::createFBO({ history_improved_texture[0], temporal_output_texture, depth_history_texture[0] }, 0u),
		bonobo::createFBO({ history_improved_texture[1], temporal_output_texture, depth_history_texture[1] }, 0u) };

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

	// Also Works for Model Indexing
	auto const fxaa_sampler = bonobo::createSampler([](GLuint sampler) {
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	});

	auto const bind_texture_with_sampler = [](GLenum target, unsigned int slot, GLuint program, std::string const& name, GLuint texture, GLuint sampler){
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, texture);
		glUniform1i(glGetUniformLocation(program, name.c_str()), static_cast<GLint>(slot));
		glBindSampler(slot, sampler);
	};


#pragma region SMAA

	// Load Special Textures
	auto const LoadSpecialTexture = [](const unsigned char data[], const auto width, const auto height, GLint internal_format, GLenum format, GLenum type) {
		GLuint texture = bonobo::createTexture(width, height, GL_TEXTURE_2D, internal_format, format, type, reinterpret_cast<GLvoid const*>(data));
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0u);

		return texture;
	};

	// Textures
	auto const edgesTex = bonobo::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_RG16, GL_RG, GL_UNSIGNED_BYTE);
	auto const blendTex = bonobo::createTexture(window_size.x, window_size.y);

	auto const areaTex = LoadSpecialTexture(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
	auto const searchTex = LoadSpecialTexture(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_UNSIGNED_BYTE);

	// FBO's
	auto const edges_fbo = bonobo::createFBO({ edgesTex }, 0u);
	auto const blend_fbo = bonobo::createFBO({ blendTex }, 0u);

#pragma endregion


	//
	// Setup lights properties
	//
	const int extra_lights = 3;
	std::array<TRSTransform<float, glm::defaultp>, constant::lights_nb + extra_lights> lightTransforms;
	std::array<glm::vec3, constant::lights_nb + extra_lights> lightColors;
	int lights_nb = static_cast<int>(constant::lights_nb) + extra_lights;
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

	lightTransforms[constant::lights_nb + 1].SetTranslate(glm::vec3(1800.0f, 125.0f, 0.0f));
	lightTransforms[constant::lights_nb + 1].SetRotate(10.97f, glm::vec3(0.0f, 1.0f, 0.0f));
	lightColors[constant::lights_nb + 1] = glm::vec3(0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
		0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)));

	lightTransforms[constant::lights_nb + 2].SetTranslate(glm::vec3(-400.0f, 125.0f, 500.0f));
	lightTransforms[constant::lights_nb + 2].SetRotate(-10.97f, glm::vec3(0.0f, 1.0f, 0.0f));
	lightColors[constant::lights_nb + 2] = glm::vec3(0.5f + 0.5f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)),
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
	auto seconds_hairball = 0.0f;

	// Other AA Vars
	enum class AA : int
	{
		NOAA = 0,
		FXAA = 1,
		SMAA = 2,
		TXAA = 3
	};

	AA aa_in_use = AA::FXAA;
	
	bool use_other_aa = false;

	// Save Vars
	bool save = false, save_steps = false, save_acc_test = false;
	int accumulation_samples = 128, current_samples = 0;
	const int FILE_NAME_SIZE = 200;
	char filename[FILE_NAME_SIZE+10] = "test";
	glm::vec2 lower_corner(-1.0f, -1.0f);
	glm::vec2 upper_corner(1.0f, 1.0f);
	float imgui_temp[4] = { lower_corner.x, lower_corner.y, upper_corner.x, upper_corner.y };
	bool show_save_area = false;

	// Ghosting Test Vars
	bool save_both = false;
	int both_test_samples = 100;
	auto const kMaxBothSaveSamples = 200;
	std::vector<glm::vec3> translations (kMaxBothSaveSamples);
	std::vector<glm::vec3> rotations (kMaxBothSaveSamples);

	// Accumulation Buffer Variables
	float accumulation_jitter_spread = 1.0f;

	// TRAA variables
	glm::mat4 currentJitter;
	glm::vec2 windowInverseResolution;
	int history_switch = 0; // Where do we write and where do we read
	float k_feedback_max = 0.979f;
	float k_feedback_min = 0.925f;
	bool use_sobel = false;

	// Load old MVPS
	for (auto& element : sponza_elements) {
		element.oldMVP = mCamera.GetWorldToClipMatrixUnjittered() * element.get_transform();
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	bool show_debug_display = false;
	GLuint temporal_time_query = 0u, sobel_time_query = 0u, deferred_time_query = 0u, fxaa_time_query= 0u, smaa_time_query = 0u;
	double ddeltatime, ddeltatimeSobel, ddeltatimeTemporal, ddeltatimeDeferred, ddeltatimeFXAA, ddeltatimeSMAA;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds(), debugLastTime;
	double fpsNextTick = lastTime + 1000.0;

	// Deferred Shading Pass
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
		&light_specular_contribution_texture,
		&clear_gbuffer_shader, &clearing_fbo,
		&depth_clearing_fbo]() {
		glDepthFunc(GL_LESS);

		//
		// Pass 0: Clear the g-buffer
		//
		glBindFramebuffer(GL_FRAMEBUFFER, depth_clearing_fbo);
		glDrawBuffers(0, 0u);
		auto const depth_clearing_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (depth_clearing_status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", depth_clearing_fbo);
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, clearing_fbo);
		GLenum const clearing_draw_buffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
		glDrawBuffers(5, clearing_draw_buffers);
		auto const clearing_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (clearing_status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", clearing_fbo);
		glViewport(0, 0, window_size.x, window_size.y);

		glDisable(GL_DITHER); // for uint writting
		//glClearDepth(1.0f);
		//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		const glm::vec4 clear_color(0.0f, 0.0f, 0.0f, 0.0f);
		const unsigned int clear_id = 0u;
		glUseProgram(clear_gbuffer_shader);
		glUniform4fv(glGetUniformLocation(clear_gbuffer_shader, "clear_color"), 1, glm::value_ptr(clear_color));
		glUniform1ui(glGetUniformLocation(clear_gbuffer_shader, "clear_id"), clear_id);
		GLStateInspection::CaptureSnapshot("Clearing Pass");
		bonobo::drawFullscreen();
		glUseProgram(0u);


		//
		// Pass 1: Render scene into the g-buffer
		//
		glBindFramebuffer(GL_FRAMEBUFFER, deferred_fbo);
		GLenum const deferred_draw_buffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		glDrawBuffers(5, deferred_draw_buffers);
		auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", deferred_fbo);
		glViewport(0, 0, window_size.x, window_size.y);


		// XXX: Is any other clearing needed?

		GLStateInspection::CaptureSnapshot("Filling Pass");

		for (auto& element : sponza_elements) {
			element.render(mCamera.GetWorldToClipMatrix(), element.get_transform(), fill_gbuffer_shader, set_uniforms);
			element.oldMVP = mCamera.GetWorldToClipMatrixUnjittered() * element.get_transform(); // World to Clip * Vertex
		}

		glEnable(GL_DITHER);  // for uint writting

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
			if (i < constant::lights_nb)
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


	auto const temporal_set_uniforms = [&windowInverseResolution, &mCamera, &currentJitter,
		&k_feedback_max, &k_feedback_min](GLuint program) {
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

	// Sobel Pass
	auto const Sobel = [&sobel_current_fbo, &sobel_shader,
		&windowInverseResolution, &mCamera,
		&window_size, &bind_texture_with_sampler,
		&default_sampler, &deferred_shading_texture,
		&depth_texture, &depth_sampler,
		&diffuse_texture, &sobel_fbo,
		&sobel_texture, &resolve_accumulation_shader,
		&history_switch, &sobel_current_texture,
		&temporal_set_uniforms, &temporal_for_Sobel_shader,
		&velocity_texture](bool use_temporal) {
		//
		// Pass: Sobel
		//
		glBindFramebuffer(GL_FRAMEBUFFER, sobel_current_fbo);
		GLenum const sobel_texture_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, sobel_texture_draw_buffers);
		auto const status_sobel_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_sobel_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", sobel_current_fbo);
		glUseProgram(sobel_shader);

		auto const sobel_uniforms = [&windowInverseResolution, &mCamera](GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
			glUniform1f(glGetUniformLocation(program, "z_near"), mCamera.mNear);
			glUniform1f(glGetUniformLocation(program, "z_far"), mCamera.mFar);
		};

		sobel_uniforms(sobel_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, sobel_shader, "deferred_texture", deferred_shading_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, sobel_shader, "depth_texture", depth_texture, depth_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 2, sobel_shader, "diffuse_texture", diffuse_texture, default_sampler);
		

		GLStateInspection::CaptureSnapshot("Sobel Pass");

		bonobo::drawFullscreen();

		glBindSampler(2, 0u);
		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);


		// Use small temporal AA for sobel filter
		glBindFramebuffer(GL_FRAMEBUFFER, sobel_fbo[(history_switch + 1) & 1]);
		GLenum const sobel_history_texture_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, sobel_history_texture_draw_buffers);
		auto const status_sobel_history_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_sobel_history_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", sobel_fbo[(history_switch + 1) & 1]);



		if (use_temporal)
		{
			glUseProgram(temporal_for_Sobel_shader);
			temporal_set_uniforms(temporal_for_Sobel_shader);
			glViewport(0, 0, window_size.x, window_size.y);
			// XXX: Is any clearing needed?
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			

			bind_texture_with_sampler(GL_TEXTURE_2D, 0, temporal_for_Sobel_shader, "sobel_current_texture", sobel_current_texture, default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 1, temporal_for_Sobel_shader, "sobel_history_texture", sobel_texture[(history_switch) & 1], default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 2, temporal_for_Sobel_shader, "velocity_texture", velocity_texture, default_sampler);


			GLStateInspection::CaptureSnapshot("Sobel AA Pass");

			bonobo::drawFullscreen();

			glBindSampler(2, 0u);
			glBindSampler(1, 0u);
			glBindSampler(0, 0u);
		}
		else
		{
			glUseProgram(resolve_accumulation_shader);

			glViewport(0, 0, window_size.x, window_size.y);
			// XXX: Is any clearing needed?
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_accumulation_shader, "accumulation_texture", sobel_current_texture, default_sampler);

			GLStateInspection::CaptureSnapshot("Sobel AA Pass");

			bonobo::drawFullscreen();

			glBindSampler(0, 0u);
		}
		glUseProgram(0u);
	};

	// Temporal Anti Aliasing
	auto const Temporal_AA = [&history_switch,
		&window_size, &bind_texture_with_sampler,
		&default_sampler,
		&depth_texture, &velocity_texture,
		&deferred_shading_texture, &sharpen_fbo,
		&diffuse_texture, &depth_sampler,
		&sharpen_shader, &windowInverseResolution,
		&temporal_output_texture, &resolve_temporal_shader,
		&ddeltatime, &currentJitter,
		&sharpen_texture, &sobel_texture,
		&depth_history_texture, &model_index_texture,
		&fxaa_sampler, &uncharted_sharpen_shader,
		&temporal_with_sobel_shader
		](GLuint temporal_shader, auto temporal_set_uniforms, const GLuint history_fbo[], const GLuint history_texture[]) {
		//
		// Pass 4: Temporal Reprojection AA
		//
		glBindFramebuffer(GL_FRAMEBUFFER, history_fbo[(history_switch + 1) & 1]);
		GLenum const history_texture_draw_buffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, history_texture_draw_buffers);
		auto const status_history_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_history_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", history_fbo[(history_switch + 1) & 1]);
		glUseProgram(temporal_shader);

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
		bind_texture_with_sampler(GL_TEXTURE_2D, 5, temporal_shader, "sobel_texture", sobel_texture[(history_switch + 1) & 1], default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 6, temporal_shader, "depth_history_texture", depth_history_texture[(history_switch) & 1], depth_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 7, temporal_shader, "model_index_texture", model_index_texture, fxaa_sampler);

		GLStateInspection::CaptureSnapshot("Temporal Pass");

		bonobo::drawFullscreen();

		glBindSampler(6, 0u);
		glBindSampler(5, 0u);
		glBindSampler(4, 0u);
		glBindSampler(3, 0u);
		glBindSampler(2, 0u);
		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);

		//
		// Pass 5: Sharpen Pass
		//
		glBindFramebuffer(GL_FRAMEBUFFER, sharpen_fbo);
		GLenum const sharpen_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, sharpen_draw_buffers);
		auto const status_sharpen_texture_buffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status_sharpen_texture_buffer != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", sharpen_fbo);

		if (temporal_with_sobel_shader == temporal_shader) {
			glUseProgram(sharpen_shader);
		}
		else {
			glUseProgram(uncharted_sharpen_shader);
		}
		

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		auto const sharpen_set_uniforms = [&windowInverseResolution](GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
		};

		if (temporal_with_sobel_shader == temporal_shader) {
			sharpen_set_uniforms(sharpen_shader);
		}
		else {
			sharpen_set_uniforms(uncharted_sharpen_shader);
		}

		

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, temporal_shader, "temporal_output", temporal_output_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("Sharpen Pass");

		bonobo::drawFullscreen();

		glBindSampler(0, 0u);
		glUseProgram(0u);

		//
		// Pass 6: Resolve Temporal Reprojection AA
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
			glUniform1f(glGetUniformLocation(program, "current_fps"), static_cast<float>(ddeltatime));
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
	};

	// Accumulation Buffer
	auto const AccumulationBuffer = [
			&accumulation_samples, &mCamera,
			&accumulation_jitter_spread, &currentJitter,
			&windowInverseResolution, &Deferred_Shading,
			&accumulation_fbo, &accumulation_shader,
			&window_size, &bind_texture_with_sampler,
			&deferred_shading_texture, &default_sampler,
			&resolve_accumulation_shader, &accumulation_texture
	]() {

		const float samples_inverse = 1.0f / static_cast<float>(accumulation_samples);

		auto const accumulation_set_uniforms = [&samples_inverse](GLuint program) {
			glUniform1f(glGetUniformLocation(program, "samples_inverse"), samples_inverse);
		};

		int old_frame_count = mCamera.frameCount;
		mCamera.frameCount = -1;
		float old_jitter_spread = mCamera.jitterSpread;
		mCamera.jitterSpread = accumulation_jitter_spread;
		bool old_jittered_projection = mCamera.jitterProjection;
		mCamera.jitterProjection = true;

		for (size_t i = 0; i < static_cast<size_t>(accumulation_samples); i++)
		{
			currentJitter = mCamera.UpdateProjection(windowInverseResolution, accumulation_samples);
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
		mCamera.frameCount = old_frame_count;
		mCamera.jitterSpread = old_jitter_spread;
		mCamera.jitterProjection = old_jittered_projection;

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
	};


	// FXAA
	auto const FXAA = [
		&fxaa_shader, &windowInverseResolution,
		&window_size, &bind_texture_with_sampler,
		&fxaa_sampler, &deferred_shading_texture]() {
		//
		// Pass: Sobel
		//
		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
		glUseProgram(fxaa_shader);

		auto const fxaa_uniforms = [&windowInverseResolution](GLuint program) {
			glUniform2f(glGetUniformLocation(program, "inv_res"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y));
		};

		fxaa_uniforms(fxaa_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, fxaa_shader, "deferred_texture", deferred_shading_texture, fxaa_sampler);

		GLStateInspection::CaptureSnapshot("FXAA Pass");

		bonobo::drawFullscreen();

		glBindSampler(0, 0u);
		glUseProgram(0u);
	};


	// SMAA
	auto const SMAA = [&edges_fbo, &blend_fbo, &window_size,
		&default_sampler, &windowInverseResolution, &smaa_edge_shader,
		&deferred_shading_texture, &bind_texture_with_sampler, &areaTex,
		&searchTex, &edgesTex, &smaa_blend_weight_shader,
		&blendTex, &smaa_blending_shader]()
	{
		// Edge Detection
		glBindFramebuffer(GL_FRAMEBUFFER, edges_fbo);
		GLenum const edge_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, edge_draw_buffers);
		auto const edges_clearing_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (edges_clearing_status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", edges_fbo);

		glUseProgram(smaa_edge_shader);

		auto const smaa_uniforms = [&windowInverseResolution, &window_size](GLuint program) {
			glUniform4f(glGetUniformLocation(program, "smaa_rt_metrics"),
				static_cast<float>(windowInverseResolution.x),
				static_cast<float>(windowInverseResolution.y),
				static_cast<float>(window_size.x),
				static_cast<float>(window_size.y));
		};

		smaa_uniforms(smaa_edge_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, smaa_edge_shader, "colorTex", deferred_shading_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("SMAA Edge Pass");

		bonobo::drawFullscreen();

		glBindSampler(0, 0u);
		glUseProgram(0u);

		// Blend Weights Calc
		glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo);
		GLenum const blend_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, blend_draw_buffers);
		auto const blend_clearing_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (blend_clearing_status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", blend_fbo);

		glUseProgram(smaa_blend_weight_shader);
		smaa_uniforms(smaa_blend_weight_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, smaa_blend_weight_shader, "edgesTex", edgesTex, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, smaa_blend_weight_shader, "areaTex", areaTex, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 2, smaa_blend_weight_shader, "searchTex", searchTex, default_sampler);

		GLStateInspection::CaptureSnapshot("SMAA Blend Weight Pass");

		bonobo::drawFullscreen();

		glBindSampler(2, 0u);
		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);

		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
		glUseProgram(smaa_blending_shader);

		smaa_uniforms(smaa_blending_shader);

		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, smaa_blending_shader, "colorTex", deferred_shading_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, smaa_blending_shader, "blendTex", blendTex, default_sampler);

		GLStateInspection::CaptureSnapshot("SMAA Blending Pass");

		bonobo::drawFullscreen();

		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);
	};

	// NOAA
	auto const NOAA = [&resolve_accumulation_shader, &window_size,
		&bind_texture_with_sampler, &resolve_no_aa_shader,
		&deferred_shading_texture, &default_sampler]()
	{
		//
		// Pass: No AA Resolve
		//
		glFinish();
		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
		glUseProgram(resolve_accumulation_shader);
		glViewport(0, 0, window_size.x, window_size.y);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_no_aa_shader, "texture_to_load", deferred_shading_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("No AA Resolve Pass");

		bonobo::drawFullscreen();

		glBindSampler(0, 0u);
		glUseProgram(0u);
	};

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
		if (!is_hairball_paused)
			seconds_hairball += static_cast<float>(ddeltatime / 1000.0);

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		// Do not move while saving
		if (!save)
		{
			mCamera.Update(ddeltatime, *inputHandler);
		}
		

		ImGui_ImplGlfwGL3_NewFrame();

		if (!save && inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

		// Hairball Rotation
		sponza_elements[sponza_elements.size() - 3].set_rotation_y(seconds_hairball * hairball_rotation_speed);

		// Sphere novement
		// The sphere is the penultimate element in vector
		sponza_elements[sponza_elements.size() - 2].set_translation(sphere_pos + glm::vec3(amplitude * std::sin(bonobo::two_pi * frequency * seconds_sphere), 0.0f, 0.0f));

		// Box Rotation
		sponza_elements[sponza_elements.size() - 1].set_rotation_y(bonobo::two_pi * box_rotation);

		// TRAA vars
		windowInverseResolution.x = 1.0f / static_cast<float>(window_size.x);
		windowInverseResolution.y = 1.0f / static_cast<float>(window_size.y);
		currentJitter = mCamera.UpdateProjection(windowInverseResolution);

		if (save && save_both)
		{
			// Pass 1-3: Deferred Shading
			bonobo::beginTimeQuery(deferred_time_query);
			Deferred_Shading();
			bonobo::endTimeQuery(deferred_time_query);

			translations[current_samples] = sponza_elements[sponza_elements.size() - 2].get_translation();
			rotations[current_samples] = sponza_elements[sponza_elements.size() - 3].get_rotation();

			// Temporal Anti Aliasing from Inside
			bonobo::beginTimeQuery(temporal_time_query);
			Temporal_AA(temporal_shader, temporal_set_uniforms, history_fbo, history_texture);
			bonobo::endTimeQuery(temporal_time_query);

			std::ostringstream samples_string;
			samples_string << std::internal << std::setfill('0') << std::setw(4) << std::to_string(current_samples);
			std::string file_name;

			file_name = std::string(filename) + "_both_" + samples_string.str() + "_no_improved";
			bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);

			// Pass: Sobel
			bonobo::beginTimeQuery(sobel_time_query);
			Sobel(use_sobel);
			bonobo::endTimeQuery(sobel_time_query);

			// Temporal Anti Aliasing modified
			Temporal_AA(temporal_with_sobel_shader, temporal_set_uniforms, history_improved_fbo, history_improved_texture);
			file_name = std::string(filename) + "_both_" + samples_string.str() + "_improved";
			bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);

			current_samples++;

			if (current_samples >= both_test_samples)
			{

				// Generate Accumulation Buffer per saved frame
				for (size_t i = 0; i < static_cast<size_t>(both_test_samples); i++)
				{
					samples_string.clear();
					samples_string.str("");

					// reload sphere position
					sponza_elements[sponza_elements.size() - 2].set_translation(translations[i]);
					// reload hairball rotation
					sponza_elements[sponza_elements.size() - 3].set_rotation(rotations[i]);

					samples_string << std::internal << std::setfill('0') << std::setw(4) << std::to_string(i);

					// Accumulation Buffer
					AccumulationBuffer();
					file_name = std::string(filename) + "_both_" + samples_string.str() + "_truth";
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);
				}

				current_samples = 0;
				save = false;
				save_both = false;
			}
		}
		else
		{
			if (use_other_aa)
			{
				bool prev_jitterProjection = mCamera.jitterProjection;
				mCamera.jitterProjection = false;
				currentJitter = mCamera.UpdateProjection(windowInverseResolution);
				bonobo::beginTimeQuery(deferred_time_query);
				Deferred_Shading();
				bonobo::endTimeQuery(deferred_time_query);
				mCamera.jitterProjection = prev_jitterProjection;

				switch (aa_in_use)
				{
				case AA::NOAA:
					NOAA();
					break;
				case AA::FXAA:
					bonobo::beginTimeQuery(fxaa_time_query);
					FXAA();
					bonobo::endTimeQuery(fxaa_time_query);
					break;
				case AA::SMAA:
					bonobo::beginTimeQuery(smaa_time_query);
					SMAA();
					bonobo::endTimeQuery(smaa_time_query);
					break;
				default:
					break;
				}
			}
			else {
				// Pass 1-3: Deferred Shading
				//debugLastTime = GetTimeMilliseconds();
				bonobo::beginTimeQuery(deferred_time_query);
				Deferred_Shading();
				//ddeltatimeDeferred = GetTimeMilliseconds() - debugLastTime;
				bonobo::endTimeQuery(deferred_time_query);

				if (use_sobel)
				{
					// Pass: Sobel
					//debugLastTime = GetTimeMilliseconds();
					bonobo::beginTimeQuery(sobel_time_query);
					Sobel(use_sobel);
					//ddeltatimeSobel = GetTimeMilliseconds() - debugLastTime;
					bonobo::endTimeQuery(sobel_time_query);

					glFlush();

					// Temporal Anti Aliasing modified
					//debugLastTime = GetTimeMilliseconds();
					bonobo::beginTimeQuery(temporal_time_query);
					Temporal_AA(temporal_with_sobel_shader, temporal_set_uniforms, history_improved_fbo, history_improved_texture);
					//ddeltatimeTemporal = GetTimeMilliseconds() - debugLastTime;
					bonobo::endTimeQuery(temporal_time_query);
				}
				else
				{
					ddeltatimeSobel = 0.0;
					// Temporal Anti Aliasing from Inside
					//debugLastTime = GetTimeMilliseconds();
					bonobo::beginTimeQuery(temporal_time_query);
					Temporal_AA(temporal_shader, temporal_set_uniforms, history_fbo, history_texture);
					//ddeltatimeTemporal = GetTimeMilliseconds() - debugLastTime;
					bonobo::endTimeQuery(temporal_time_query);
				}
			}
		}

		
		// Accumulation Save Test
		if (save && save_acc_test)
		{
			if (current_samples < CAMERA_JITTERING_SIZE)
			{
				if (save_steps)
				{
					std::string file_name(filename);
					file_name += "_temporal_" + std::to_string(current_samples);
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);
				}

				if (current_samples == CAMERA_JITTERING_SIZE - 1)
				{
					if (!save_steps)
					{
						std::string file_name(filename);
						file_name += "_temporal" ;
						bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);
					}

					// Save Accumulation Buffer
					AccumulationBuffer();
					std::string file_name(filename);
					file_name += "_ground_truth";
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);


					// Reder Without Jitter
					bool prev_jitterProjection = mCamera.jitterProjection;
					mCamera.jitterProjection = false;
					currentJitter = mCamera.UpdateProjection(windowInverseResolution);
					Deferred_Shading();
					mCamera.jitterProjection = prev_jitterProjection;

					// Save NOAA
					NOAA();
					file_name = std::string(filename);
					file_name += "_no_aa";
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);

					// Save FXAA
					// We take advantage that we just rendered the unjittered screen
					FXAA();
					file_name = std::string(filename);
					file_name += "_fxaa";
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);

					// Save SMAA
					// We take advantage that we just rendered the unjittered screen
					SMAA();
					file_name = std::string(filename);
					file_name += "_smaa";
					bonobo::screenShot(file_name, lower_corner, upper_corner, window_size);

					save = false;
					save_acc_test = false;
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

		if (show_debug_display)
		{
			//
			// Output content of the g-buffer as well as of the shadowmap, for debugging purposes
			//
			bonobo::displayTexture({ -0.95f, -0.95f }, { -0.55f, -0.55f }, diffuse_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ -0.45f, -0.95f }, { -0.05f, -0.55f }, specular_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ 0.05f, -0.95f }, { 0.45f, -0.55f }, normal_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ 0.55f, -0.95f }, { 0.95f, -0.55f }, depth_texture, default_sampler, { 0, 0, 0, -1 }, window_size, &mCamera);
			bonobo::displayTexture({ -0.95f,  0.55f }, { -0.55f,  0.95f }, shadowmap_texture, default_sampler, { 0, 0, 0, -1 }, window_size, &mCamera);
			bonobo::displayTexture({ -0.45f,  0.55f }, { -0.05f,  0.95f }, light_diffuse_contribution_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ 0.05f,  0.55f }, { 0.45f,  0.95f }, light_specular_contribution_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ 0.55f,  0.55f }, { 0.95f,  0.95f }, velocity_texture, default_sampler, { 0, 1, 2, -1 }, window_size);
			bonobo::displayTexture({ 0.55f,  0.10f }, { 0.95f,  0.50f }, sobel_texture[(history_switch + 1) & 1], default_sampler, { 0, 1, 2, -1 }, window_size);
			//bonobo::displayTexture({ -1.0f,  -1.0f }, { 1.0f,  1.0f }, sobel_texture[(history_switch + 1) & 1], default_sampler, { 0, 1, 2, -1 }, window_size);
			//bonobo::displayTexture({ -1.0f,  -1.0f }, { 1.0f,  1.0f }, edgesTex, default_sampler, { 0, 1, 2, -1 }, window_size);
			
		}
		
		if (show_save_area)
		{
			bonobo::displayTexture(lower_corner, upper_corner, sobel_texture[(history_switch + 1) & 1], default_sampler, { 0, 1, 2, -1 }, window_size);
		}
		

		//
		// Reset viewport back to normal
		//
		glViewport(0, 0, window_size.x, window_size.y);

		GLStateInspection::View::Render();
		Log::View::Render();

		// Collect Timers
		bonobo::collectTimeQuery(temporal_time_query, ddeltatimeTemporal);
		bonobo::collectTimeQuery(deferred_time_query, ddeltatimeDeferred);
		if (use_sobel)
		{
			bonobo::collectTimeQuery(sobel_time_query, ddeltatimeSobel);
		}
		else
		{
			ddeltatimeSobel = 0.0;
		}

		if (use_other_aa)
		{
			switch (aa_in_use)
			{
			case AA::FXAA:
				bonobo::collectTimeQuery(fxaa_time_query, ddeltatimeFXAA);
				break;
			case AA::SMAA:
				bonobo::collectTimeQuery(smaa_time_query, ddeltatimeSMAA);
				break;
			case AA::NOAA:
			case AA::TXAA:
			default:
				break;
			}
		}
		else {
			ddeltatimeFXAA = 0.0;
			ddeltatimeSMAA = 0.0;
		}
		

		bool opened = ImGui::Begin("Render Time", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened)
		{
			ImGui::Text("Last Frame: %.3f ms", ddeltatime);
			ImGui::Text("Deferred: %.3f ms", ddeltatimeDeferred);
			if (use_other_aa)
			{
				switch (aa_in_use)
				{
				case AA::FXAA:
					ImGui::Text("FXAA: %.3f ms", ddeltatimeFXAA);
					break;
				case AA::SMAA:
					ImGui::Text("SMAA: %.3f ms", ddeltatimeSMAA);
					break;
				case AA::NOAA:
				case AA::TXAA:
				default:
					break;
				}
			}
			else
			{
				if (use_sobel)
				{
					ImGui::Text("Sobel: %.3f ms", ddeltatimeSobel);
				}
				ImGui::Text("Temporal: %.3f ms", ddeltatimeTemporal);
			}
			
		}
		ImGui::End();

		opened = ImGui::Begin("Debug", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened)
		{
			ImGui::Checkbox("Show Debug Displays?", &show_debug_display);
			
		}
		ImGui::End();

		if (!save)
		{
			opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(120, 50), -1.0f, 0);
			if (opened) {
				ImGui::Checkbox("Use Other AA: ", &use_other_aa);
				if (!use_other_aa)
				{
					ImGui::Checkbox("Use Sobel Shader?", &use_sobel);
					ImGui::Checkbox("Jitter?", &mCamera.jitterProjection);
					ImGui::SliderFloat("Jitter Spread", &mCamera.jitterSpread, 0.0f, 2.0f);
					ImGui::SliderFloat("k_feedback_min", &k_feedback_min, 0.0f, 1.0f);
					ImGui::SliderFloat("k_feedback_max", &k_feedback_max, 0.0f, 1.0f);
					aa_in_use = AA::TXAA;
				}
				else
				{
					if (aa_in_use == AA::TXAA)
					{
						aa_in_use = AA::NOAA;
					}
					ImGui::SliderInt("0:NOAA, 1:FXAA, 2:SMAA", reinterpret_cast<int *>(&aa_in_use), static_cast<int>(AA::NOAA), static_cast<int>(AA::SMAA));
				}
				ImGui::Checkbox("Pause lights", &are_lights_paused);
				ImGui::SliderInt("Number of lights", &lights_nb, 1, static_cast<int>(constant::lights_nb) + extra_lights);
				ImGui::SliderFloat("Box Rotation", &box_rotation, 0.0f, 1.0f);
				ImGui::Checkbox("Pause Hairball", &is_hairball_paused);
				ImGui::SliderFloat("Hairball Rotation Speed", &hairball_rotation_speed, 0.0f, 1.0f);
				ImGui::Checkbox("Pause sphere", &is_sphere_paused);
				ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 1000.0f);
				ImGui::SliderFloat("Frequency", &frequency, 0.0f, 2.0f);
				ImGui::SliderFloat3("Sphere Home Position", glm::value_ptr(sphere_pos), -2000.0f, 2000.0f);
			}
			ImGui::End();

			opened = ImGui::Begin("Save Image", nullptr, ImVec2(120, 50), -1.0f, 0);
			if (opened)
			{
				ImGui::InputText("Filename", filename, FILE_NAME_SIZE);
				ImGui::Checkbox("Save Steps", &save_steps);
				ImGui::SliderInt("Sample Amount", &accumulation_samples, 1, CAMERA_JITTERING_SIZE);
				ImGui::SliderFloat("Accumulation Jitter Spread", &accumulation_jitter_spread, 0.0f, 2.0f);
				imgui_temp[0] = lower_corner.x;
				imgui_temp[1] = lower_corner.y;
				imgui_temp[2] = upper_corner.x;
				imgui_temp[3] = upper_corner.y;
				ImGui::SliderFloat4("Lower and Upper Corner", imgui_temp, -1.0f, 1.0f);
				lower_corner.x = imgui_temp[0];
				lower_corner.y = imgui_temp[1];
				upper_corner.x = std::max(imgui_temp[2], imgui_temp[0]);
				upper_corner.y = std::max(imgui_temp[3], imgui_temp[1]);
				ImGui::Checkbox("Show Save Area", &show_save_area);
				if (ImGui::Button("Save Image"))
				{
					save = true;
					save_acc_test = true;
					current_samples = 0;

					// Save Current Info
					bonobo::saveConfig(filename, use_other_aa, static_cast<int>(aa_in_use),
						use_sobel, mCamera,
						k_feedback_min, k_feedback_max,
						accumulation_samples, accumulation_jitter_spread,
						lower_corner, upper_corner);
				}
				ImGui::SliderInt("Both Test Samples", &both_test_samples, 1, kMaxBothSaveSamples);
				if (ImGui::Button("Save Both"))
				{
					save = true;
					save_both = true;
					current_samples = 0;

					// Save Current Info
					bonobo::saveConfig(filename, use_other_aa, static_cast<int>(aa_in_use),
						use_sobel, mCamera,
						k_feedback_min, k_feedback_max,
						accumulation_samples, accumulation_jitter_spread,
						lower_corner, upper_corner, true, both_test_samples);
				}
			}
			ImGui::End();
		}

		ImGui::Render();

		window->Swap();

		history_switch++;
		if (history_switch >= 2) history_switch -= 2;

		lastTime = nowTime;
	}

	bonobo::destroyTimeQuery(temporal_time_query);
	bonobo::destroyTimeQuery(sobel_time_query);
	bonobo::destroyTimeQuery(deferred_time_query);
	bonobo::destroyTimeQuery(fxaa_time_query);
	bonobo::destroyTimeQuery(smaa_time_query);


	glDeleteProgram(smaa_edge_shader);
	smaa_edge_shader = 0u;
	glDeleteProgram(smaa_blending_shader);
	smaa_blending_shader = 0u;
	glDeleteProgram(smaa_blend_weight_shader);
	smaa_blend_weight_shader = 0u;
	glDeleteProgram(fxaa_shader);
	fxaa_shader = 0u;
	glDeleteProgram(temporal_for_Sobel_shader);
	temporal_for_Sobel_shader = 0u;
	glDeleteProgram(temporal_with_sobel_shader);
	temporal_with_sobel_shader = 0u;
	glDeleteProgram(sobel_shader);
	sobel_shader = 0u;
	glDeleteProgram(resolve_no_aa_shader);
	resolve_no_aa_shader = 0u;
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
