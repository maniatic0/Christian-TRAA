#pragma once

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "core/FPSCamera.h" // As it includes OpenGL headers, import it after glad

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <fstream>

//! \brief Namespace containing a few helpers for the LUGG computer graphics labs.
namespace bonobo
{
	//! \brief Formalise mapping between an OpenGL VAO attribute binding,
	//!        and the meaning of that attribute.
	enum class shader_bindings : unsigned int{
		vertices = 0u, //!< = 0, value of the binding point for vertices
		normals,       //!< = 1, value of the binding point for normals
		texcoords,     //!< = 2, value of the binding point for texcoords
		tangents,      //!< = 3, value of the binding point for tangents
		binormals      //!< = 4, value of the binding point for binormals
	};

	//! \brief Association of a sampler name used in GLSL to a
	//!        corresponding texture ID.
	using texture_bindings = std::unordered_map<std::string, GLuint>;

	//! \brief Contains the data for a mesh in OpenGL.
	struct mesh_data {
		GLuint vao;                //!< OpenGL name of the Vertex Array Object
		GLuint bo;                 //!< OpenGL name of the Buffer Object
		GLuint ibo;                //!< OpenGL name of the Buffer Object for indices
		size_t vertices_nb;        //!< number of vertices stored in bo
		size_t indices_nb;         //!< number of indices stored in ibo
		texture_bindings bindings; //!< texture bindings for this mesh
		GLenum drawing_mode;       //!< OpenGL drawing mode, i.e. GL_TRIANGLES, GL_LINES, etc.

		mesh_data() : vao(0u), bo(0u), ibo(0u), vertices_nb(0u), indices_nb(0u), bindings(), drawing_mode(GL_TRIANGLES)
		{
		}
	};

	//! \brief Allocate some objects needed by some helper functions.
	void init();

	//! \brief Deallocate objects allocated by the `init()` function.
	void deinit();

	//! \brief Load objects found in an object/scene file, using assimp.
	//!
	//! @param [in] filename of the object/scene file to load, relative to
	//!             the `res/scenes` folder
	//! @param [in] if its going to look for materials in the testing folder
	//! @return a vector of filled in `mesh_data` structures, one per
	//!         object found in the input file
	std::vector<mesh_data> loadObjects(std::string const& filename, bool use_testing = false);

	//! \brief Creates an OpenGL texture without any content nor parameterised.
	//!
	//! @param [in] width width of the texture to create
	//! @param [in] height height of the texture to create
	//! @param [in] target OpenGL texture target to create, i.e.
	//!             GL_TEXTURE_2D & co.
	//! @param [in] internal_format formatting of the texture, i.e. how many
	//!             channels
	//! @param [in] format formatting of the pixel data, i.e. in which
	//!             layout are the channels stored
	//! @param [in] type data type of the pixel data
	//! @param [in] data what to put in the texture
	GLuint createTexture(uint32_t width, uint32_t height,
	                     GLenum target = GL_TEXTURE_2D,
	                     GLint internal_format = GL_RGBA,
	                     GLenum format = GL_RGBA,
	                     GLenum type = GL_UNSIGNED_BYTE,
	                     GLvoid const* data = nullptr);

	//! \brief Load a PNG image into an OpenGL 2D-texture.
	//!
	//! @param [in] filename of the PNG image, relative to the `textures`
	//!             folder within the `resources` folder.
	//! @param [in] generate_mipmap whether or not to generate a mipmap hierarchy
	//! @return the name of the OpenGL 2D-texture
	GLuint loadTexture2D(std::string const& filename,
	                     bool generate_mipmap = true);

	//! \brief Load six PNG images into an OpenGL cubemap-texture.
	//!
	//! @param [in] posx path to the texture on the left of the cubemap
	//! @param [in] negx path to the texture on the right of the cubemap
	//! @param [in] posy path to the texture on the top of the cubemap
	//! @param [in] negy path to the texture on the bottom of the cubemap
	//! @param [in] posz path to the texture on the back of the cubemap
	//! @param [in] negz path to the texture on the front of the cubemap
	//! @param [in] generate_mipmap whether or not to generate a mipmap hierarchy
	//! @return the name of the OpenGL cubemap-texture
	//!
	//! All paths are relative to the `res/cubemaps` folder.
	GLuint loadTextureCubeMap(std::string const& posx, std::string const& negx,
                                  std::string const& posy, std::string const& negy,
                                  std::string const& posz, std::string const& negz,
                                  bool generate_mipmap = true);

	//! \brief Create an OpenGL program consisting of a vertex and a
	//!        fragment shader.
	//!
	//! @param [in] vert_shader_source_path of the vertex shader source
	//!             code, relative to the `shaders/EDAF80` folder
	//! @param [in] frag_shader_source_path of the fragment shader source
	//!             code, relative to the `shaders/EDAF80` folder
	//! @return the name of the OpenGL shader program
	GLuint createProgram(std::string const& vert_shader_source_path,
	                     std::string const& frag_shader_source_path);

	//! \brief Display the current texture in the specified rectangle.
	//!
	//! @param [in] lower_left the lower left corner of the rectangle
	//!             containing the texture
	//! @param [in] upper_right the upper rigth corner of the rectangle
	//!             containing the texture
	//! @param [in] texture the OpenGL name of the texture to display
	//! @param [in] sampler the OpenGL name of the sampler to use
	//! @param [in] swizzle how to mix in the different channels, for
	//!             example (0, 2, 1, -1) will swap the green and blue
	//!             channels as well as invalidating (setting it to 1) the
	//!             alpha channel
	//! @param [in] window_size the size in pixels of the main window, the
	//!             one relative to which you want to draw this texture
	void displayTexture(glm::vec2 const& lower_left,
	                    glm::vec2 const& upper_right, GLuint texture,
	                    GLuint samper, glm::ivec4 const& swizzle,
	                    glm::ivec2 const& window_size,
	                    FPSCameraf const* camera = nullptr);

	//! \brief Create an OpenGL FrameBuffer Object using the specified
	//!        attachments.
	//!
	//! @param [in] color_attachments a vector of all the texture to bind
	//!             as color attachment, i.e. not as depth texture
	//! @param [in] depth_attachment a texture, if any, to use as depth
	//!             attachment
	//! @return the name of the OpenGL FBO
	GLuint createFBO(std::vector<GLuint> const& color_attachments,
	                 GLuint depth_attachment = 0u);

	//! \brief Create an OpenGL sampler and set it up.
	//!
	//! @param [in] setup a lambda function to parameterise the sampler
	//! @return the name of the OpenGL sampler
	GLuint createSampler(std::function<void (GLuint)> const& setup);

	//! \brief Draw full screen.
	void drawFullscreen();

	//! \brief Get vector squared magnitude
	float sqrMagnitude(glm::vec3 const v);

	//! \brief Get vector magnitude
	float magnitude(glm::vec3 const v);

	//! \brief Take screenshot of opengl frame and saves it to file_name in a png
	//! 
	//! @param [in] lower_corner [-1,1]*[-1,1] represents the lower corner to take a screenshot
	//! @param [in] upper_corner [-1,1]*[-1,1] represents the upper corner to take a screenshot
	//! @param [in] windows_size is the current size of the screen
	void screenShot(std::string file_name, const glm::vec2 &lower_corner, const glm::vec2 &upper_corner, const glm::vec2 &windows_size);

	//! \brief Save Current Configure Values
	//! 
	//! @param [in] file_name to save the configuration
	//! @param [in] using_sobel if we are using sobel shader
	//! @param [in] camera Current Camera
	//! @param [in] k_feedback_min feedback minimun value
	//! @param [in] k_feedback_max feedback maximun value
	//! @param [in] sample_amount amount of samples for the accumulation buffer
	//! @param [in] accumulation_jitter_spread spread of jitter for the accumulation buffer
	//! @param [in] lower_corner lower corner of the save area
	//! @param [in] upper_corner upper corner of the save area
	//! @param [in] ghosting_test if this is a ghosting test
	//! @param [in] ghosting_test_amount amount of samples taken in ghosting test
	void saveConfig(std::string file_name,
		const bool &using_sobel, FPSCameraf &camera,
		const float &k_feedback_min, const float &k_feedback_max,
		const int &sample_amount, const float &accumulation_jitter_spread,
		const glm::vec2 &lower_corner, const glm::vec2 &upper_corner,
		const bool &ghosting_test = false, const int &ghosting_test_amount = 0);

	//! \brief Insert time query inGPU pipeline
	//! 
	//! @param [in] where to store query id
	void beginTimeQuery(GLuint &query);

	//! \brief Stop timer
	//! 
	//! @param [in] where the query id is stored
	void endTimeQuery(const GLuint &query);

	//! \brief Get the time elapsed since the query started
	//! 
	//! @param [in] where the query id is stored
	//! @param [in] where to store the time elapsed, in miliseconds
	//! @param [in] if we wait to make sure the results are available
	void collectTimeQuery(const GLuint &query, double &time, const bool wait_results = false);

	//! \brief Get the time elapsed since the query started
	//! 
	//! @param [in] where the query id is stored
	void destroyTimeQuery(GLuint &query);
}
