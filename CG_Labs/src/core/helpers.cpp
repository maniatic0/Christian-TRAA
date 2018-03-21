#include "config.hpp"
#include "helpers.hpp"

#include "core/Log.h"
#include "core/Misc.h"
#include "core/opengl.hpp"
#include "core/various.hpp"
#include "external/lodepng.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include <cassert>

namespace local
{
	static GLuint fullscreen_shader;
	static GLuint display_vao;
}

void
bonobo::init()
{
	glGenVertexArrays(1, &local::display_vao);
	assert(local::display_vao != 0u);
	local::fullscreen_shader = bonobo::createProgram("fullscreen.vert", "fullscreen.frag");
	if (local::fullscreen_shader == 0u)
		LogError("Failed to load \"fullscreen.vert\" and \"fullscreen.frag\"");
}

void
bonobo::deinit()
{
	glDeleteVertexArrays(1, &local::display_vao);
}

static std::vector<u8>
getTextureData(std::string const& filename, u32& width, u32& height, bool flip)
{
	auto const path = config::resources_path(filename);
	std::vector<unsigned char> image;
	if (lodepng::decode(image, width, height, path, LCT_RGBA) != 0) {
		LogWarning("Couldn't load or decode image file %s", path.c_str());
		return image;
	}
	if (!flip)
		return image;

	auto const channels_nb = 4u;
	auto flipBuffer = std::vector<u8>(width * height * channels_nb);
	for (u32 y = 0; y < height; y++)
		memcpy(flipBuffer.data() + (height - 1 - y) * width * channels_nb, &image[y * width * channels_nb], width * channels_nb);

	return flipBuffer;
}

std::vector<bonobo::mesh_data>
bonobo::loadObjects(std::string const& filename, bool use_testing)
{
	std::vector<bonobo::mesh_data> objects;

	std::string scene_filepath;
	if (!use_testing)
	{
		scene_filepath = config::resources_path("scenes/" + filename);
	}
	else
	{
		scene_filepath = config::resources_path("../testing/models/" + filename);
	}
	LogInfo("Loading \"%s\"", scene_filepath.c_str());
	Assimp::Importer importer;
	auto const assimp_scene = importer.ReadFile(scene_filepath, aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_CalcTangentSpace);
	if (assimp_scene == nullptr || assimp_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimp_scene->mRootNode == nullptr) {
		LogError("Assimp failed to load \"%s\": %s", scene_filepath.c_str(), importer.GetErrorString());
		return objects;
	}


	if (assimp_scene->mNumMeshes == 0u) {
		LogError("No mesh available; loading \"%s\" must have had issues", scene_filepath.c_str());
		return objects;
	}

	std::vector<texture_bindings> materials_bindings;
	materials_bindings.reserve(assimp_scene->mNumMaterials);

	LogInfo("\t* materials");
	for (size_t i = 0; i < assimp_scene->mNumMaterials; ++i) {
		texture_bindings bindings;
		auto const material = assimp_scene->mMaterials[i];

		auto const process_texture = [&bindings,&material,&use_testing, i](aiTextureType type, std::string const& type_as_str, std::string const& name){
			if (material->GetTextureCount(type)) {
				if (material->GetTextureCount(type) > 1)
					LogWarning("Material %d has more than one %s texture: discarding all but the first one.", i, type_as_str.c_str());
				aiString path;
				material->GetTexture(type, 0, &path);
				GLuint id;
				if (!use_testing)
				{
					id = bonobo::loadTexture2D("../crysponza/" + std::string(path.C_Str()), type_as_str != "opacity");
				}
				else
				{
					id = bonobo::loadTexture2D("../../testing/models/" + std::string(path.C_Str()), type_as_str != "opacity");
				}
				
				if (id != 0u)
					bindings.emplace(name, id);
			}
		};

		process_texture(aiTextureType_DIFFUSE,  "diffuse",  "diffuse_texture");
		process_texture(aiTextureType_SPECULAR, "specular", "specular_texture");
		process_texture(aiTextureType_NORMALS,  "normals",  "normals_texture");
		process_texture(aiTextureType_OPACITY,  "opacity",  "opacity_texture");

		materials_bindings.push_back(bindings);
	}

	LogInfo("\t* meshes");
	objects.reserve(assimp_scene->mNumMeshes);
	for (size_t j = 0; j < assimp_scene->mNumMeshes; ++j) {
		auto const assimp_object_mesh = assimp_scene->mMeshes[j];

		if (!assimp_object_mesh->HasFaces()) {
			LogError("Unsupported object \"%s\": has no faces", assimp_object_mesh->mName.C_Str());
			continue;
		}
		if ((assimp_object_mesh->mPrimitiveTypes & ~static_cast<uint32_t>(aiPrimitiveType_POINT))    != 0u
		 && (assimp_object_mesh->mPrimitiveTypes & ~static_cast<uint32_t>(aiPrimitiveType_LINE))     != 0u
		 && (assimp_object_mesh->mPrimitiveTypes & ~static_cast<uint32_t>(aiPrimitiveType_TRIANGLE)) != 0u) {
			LogError("Unsupported object \"%s\": uses multiple primitive types", assimp_object_mesh->mName.C_Str());
			continue;
		}
		if ((assimp_object_mesh->mPrimitiveTypes & static_cast<uint32_t>(aiPrimitiveType_POLYGON)) == static_cast<uint32_t>(aiPrimitiveType_POLYGON)) {
			LogError("Unsupported object \"%s\": uses polygons", assimp_object_mesh->mName.C_Str());
			continue;
		}
		if (!assimp_object_mesh->HasPositions()) {
			LogError("Unsupported object \"%s\": has no positions", assimp_object_mesh->mName.C_Str());
			continue;
		}

		bonobo::mesh_data object;

		glGenVertexArrays(1, &object.vao);
		assert(object.vao != 0u);
		glBindVertexArray(object.vao);

		auto const vertices_offset = 0u;
		auto const vertices_size = static_cast<GLsizeiptr>(assimp_object_mesh->mNumVertices * sizeof(glm::vec3));

		auto const normals_offset = vertices_size;
		auto const normals_size = assimp_object_mesh->HasNormals() ? vertices_size : 0u;

		auto const texcoords_offset = normals_offset + normals_size;
		auto const texcoords_size = assimp_object_mesh->HasTextureCoords(0u) ? vertices_size : 0u;

		auto const tangents_offset = texcoords_offset + texcoords_size;
		auto const tangents_size = assimp_object_mesh->HasTangentsAndBitangents() ? vertices_size : 0u;

		auto const binormals_offset = tangents_offset + tangents_size;
		auto const binormals_size = assimp_object_mesh->HasTangentsAndBitangents() ? vertices_size : 0u;

		auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		                                            +normals_size
		                                            +texcoords_size
		                                            +tangents_size
		                                            +binormals_size
		                                            );
		glGenBuffers(1, &object.bo);
		assert(object.bo != 0u);
		glBindBuffer(GL_ARRAY_BUFFER, object.bo);
		glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(assimp_object_mesh->mVertices));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

		if (assimp_object_mesh->HasNormals()) {
			glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(assimp_object_mesh->mNormals));
			glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
			glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));
		}

		if (assimp_object_mesh->HasTextureCoords(0u)) {
			glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(assimp_object_mesh->mTextureCoords[0u]));
			glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
			glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));
		}

		if (assimp_object_mesh->HasTangentsAndBitangents()) {
			glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(assimp_object_mesh->mTangents));
			glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
			glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

			glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(assimp_object_mesh->mBitangents));
			glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
			glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0u);

		auto const num_vertices_per_face = assimp_object_mesh->mFaces[0u].mNumIndices;
		object.indices_nb = assimp_object_mesh->mNumFaces * num_vertices_per_face;
		auto object_indices = std::make_unique<GLuint[]>(static_cast<size_t>(object.indices_nb));
		for (size_t i = 0u; i < assimp_object_mesh->mNumFaces; ++i) {
			auto const& face = assimp_object_mesh->mFaces[i];
			assert(face.mNumIndices <= 3);
			object_indices[num_vertices_per_face * i + 0u] = face.mIndices[0u];
			if (num_vertices_per_face >= 1u)
				object_indices[num_vertices_per_face * i + 1u] = face.mIndices[1u];
			if (num_vertices_per_face >= 2u)
				object_indices[num_vertices_per_face * i + 2u] = face.mIndices[2u];
		}
		glGenBuffers(1, &object.ibo);
		assert(object.ibo != 0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<unsigned int>(object.indices_nb) * sizeof(GL_UNSIGNED_INT), reinterpret_cast<GLvoid const*>(object_indices.get()), GL_STATIC_DRAW);
		object_indices.reset(nullptr);

		glBindVertexArray(0u);
		glBindBuffer(GL_ARRAY_BUFFER, 0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

		auto const material_id = assimp_object_mesh->mMaterialIndex;
		if (material_id >= materials_bindings.size())
			LogError("Object \"%s\" has a material index of %u, but only %u materials were retrieved.", assimp_object_mesh->mName.C_Str(), material_id, materials_bindings.size());
		else
			object.bindings = materials_bindings[material_id];

		objects.push_back(object);

//		LogInfo("Loaded object \"%s\" with normals:%d, tangents&bitangents:%d, texcoords:%d",
//		        assimp_object_mesh->mName.C_Str(), assimp_object_mesh->HasNormals(),
//		        assimp_object_mesh->HasTangentsAndBitangents(), assimp_object_mesh->HasTextureCoords(0));
	}

	return objects;
}

GLuint
bonobo::createTexture(uint32_t width, uint32_t height, GLenum target, GLint internal_format, GLenum format, GLenum type, GLvoid const* data)
{
	GLuint texture = 0u;
	glGenTextures(1, &texture);
	assert(texture != 0u);
	glBindTexture(target, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(target, 0, internal_format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);
	glBindTexture(target, 0u);

	return texture;
}

GLuint
bonobo::loadTexture2D(std::string const& filename, bool generate_mipmap)
{
	u32 width, height;
	auto const data = getTextureData("textures/" + filename, width, height, true);
	if (data.empty())
		return 0u;

	GLuint texture = bonobo::createTexture(width, height, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<GLvoid const*>(data.data()));
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generate_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (generate_mipmap)
		glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0u);

	return texture;
}

GLuint
bonobo::loadTextureCubeMap(std::string const& posx, std::string const& negx,
                           std::string const& posy, std::string const& negy,
                           std::string const& posz, std::string const& negz,
                           bool generate_mipmap)
{
	GLuint texture = 0u;
	// Create an OpenGL texture object. Similarly to `glGenVertexArrays()`
	// and `glGenBuffers()` that were used in assignment 2,
	// `glGenTextures()` can create `n` texture objects at once. Here we
	// only one texture object that will contain our whole cube map.
	glGenTextures(1, /*! \todo fill me */&texture);
	assert(texture != 0u);

	// Similarly to vertex arrays and buffers, we first need to bind the
	// texture object in orther to use it. Here we will bind it to the
	// GL_TEXTURE_CUBE_MAP target to indicate we want a cube map. If you
	// look at `bonobo::loadTexture2D()` just above, you will see that
	// GL_TEXTURE_2D is used there, as we want a simple 2D-texture.
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	// Set the wrapping properties of the texture; you can have a look on
	// http://docs.gl to learn more about them
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set the minification and magnification properties of the textures;
	// you can have a look on http://docs.gl to lear more about them, or
	// attend EDAN35 in the next period ;-)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, generate_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// We need to fill in the cube map using the images passed in as
	// argument. The function `getTextureData()` uses lodepng to read in
	// the image files and return a `std::vector<u8>` containing all the
	// texels.
	u32 width, height;
	auto data = getTextureData("cubemaps/" + negx, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_NEGATIVE_X.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	             /* mipmap level, you'll see that in EDAN35 */0,
	             /* how are the components internally stored */GL_RGBA,
	             /* the width of the cube map's face */static_cast<GLsizei>(width),
	             /* the height of the cube map's face */static_cast<GLsizei>(height),
	             /* must always be 0 */0,
	             /* the format of the pixel data: which components are available */GL_RGBA,
	             /* the type of each component */GL_UNSIGNED_BYTE,
	             /* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + posx, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_POSITIVE_X.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + negy, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_NEGATIVE_Y.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + posy, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_POSITIVE_Y.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + negz, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_NEGATIVE_Z.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + posz, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `bonobo::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_POSITIVE_Z.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	//! \todo repeat now the texture filling for the 5 remaining faces

	if (generate_mipmap)
		// Generate the mipmap hierarchy; wait for EDAN35 to understand
		// what it does
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0u);

	return texture;
}

GLuint
bonobo::createProgram(std::string const& vert_shader_source_path, std::string const& frag_shader_source_path)
{
	auto const vertex_shader_source = utils::slurp_file(config::shaders_path("EDAF80/" + vert_shader_source_path));
	GLuint vertex_shader = utils::opengl::shader::generate_shader(GL_VERTEX_SHADER, vertex_shader_source);
	if (vertex_shader == 0u)
		return 0u;

	auto const fragment_shader_source = utils::slurp_file(config::shaders_path("EDAF80/" + frag_shader_source_path));
	GLuint fragment_shader = utils::opengl::shader::generate_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
	if (fragment_shader == 0u)
		return 0u;

	GLuint program = utils::opengl::shader::generate_program({ vertex_shader, fragment_shader });
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}

void
bonobo::displayTexture(glm::vec2 const& lower_left, glm::vec2 const& upper_right, GLuint texture, GLuint sampler, glm::ivec4 const& swizzle, glm::ivec2 const& window_size, FPSCameraf const* camera)
{
	auto const relative_to_absolute = [](float coord, int size) {
		return static_cast<GLint>((coord + 1.0f) / 2.0f * size);
	};
	auto const viewport_origin = glm::ivec2(relative_to_absolute(lower_left.x, window_size.x),
	                                        relative_to_absolute(lower_left.y, window_size.y));
	auto const viewport_size = glm::ivec2(relative_to_absolute(upper_right.x, window_size.x),
	                                      relative_to_absolute(upper_right.y, window_size.y))
	                         - viewport_origin;

	int const linearise = camera != nullptr;

	glViewport(viewport_origin.x, viewport_origin.y, viewport_size.x, viewport_size.y);
	glUseProgram(local::fullscreen_shader);
	glBindVertexArray(local::display_vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(0, sampler);
	glUniform1i(glGetUniformLocation(local::fullscreen_shader, "tex"), 0);
	glUniform4iv(glGetUniformLocation(local::fullscreen_shader, "swizzle"), 1, glm::value_ptr(swizzle));
	glUniform1i(glGetUniformLocation(local::fullscreen_shader, "linearise"), linearise);
	glUniform1f(glGetUniformLocation(local::fullscreen_shader, "near"), linearise ? camera->mNear : 0.0f);
	glUniform1f(glGetUniformLocation(local::fullscreen_shader, "far"), linearise ? camera->mFar : 0.0f);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindSampler(0, 0u);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

GLuint
bonobo::createFBO(std::vector<GLuint> const& color_attachments, GLuint depth_attachment)
{
	auto const attach = [](GLenum attach_point, GLuint attachment){
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach_point, GL_TEXTURE_2D, attachment, 0);
		auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Failed to attach %u at %u", attachment, attach_point);
	};

	GLuint fbo = 0u;
	glGenFramebuffers(1, &fbo);
	assert(fbo != 0u);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (size_t i = 0; i < color_attachments.size(); ++i)
		attach(static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i), color_attachments[i]);
	if (depth_attachment != 0u)
		attach(GL_DEPTH_ATTACHMENT, depth_attachment);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fbo;
}

GLuint
bonobo::createSampler(std::function<void (GLuint)> const& setup)
{
	GLuint sampler = 0u;
	glGenSamplers(1, &sampler);
	assert(sampler != 0u);
	setup(sampler);
	return sampler;
}

void
bonobo::drawFullscreen()
{
	glBindVertexArray(local::display_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0u);
}


// Get vector squared magnitude
float bonobo::sqrMagnitude(glm::vec3 const v)
{
	auto const x = v.x * v.x;
	auto const y = v.y * v.y;
	auto const z = v.z * v.z;
	return x + y + z;
}

// Get vector magnitude
float bonobo::magnitude(glm::vec3 const v)
{
	return std::sqrt(bonobo::sqrMagnitude(v));
}


void threadSaveScreenshot(std::string file_name, int width, int height, unsigned char* image)
{
	file_name = config::resources_path(file_name + ".png");
	std::string printing_string = "Saving: " + file_name;
	Log(printing_string.c_str());

	size_t temp_y, temp_inv_y, temp_x, pos, inv_pos;

	// Image is inverted for png saving
	for (size_t y = 0; y < height/2; y++)
	{
		temp_y = 4 * width * y;
		temp_inv_y = 4 * width * (height - 1 - y);
		for (size_t x = 0; x < width; x++)
		{
			temp_x = 4 * x;
			pos = temp_y + temp_x;
			inv_pos = temp_inv_y + temp_x;
			std::swap(image[pos + 0], image[inv_pos + 0]);
			std::swap(image[pos + 1], image[inv_pos + 1]);
			std::swap(image[pos + 2], image[inv_pos + 2]);
			std::swap(image[pos + 3], image[inv_pos + 3]);
		}
	}


	std::vector<unsigned char> png;

	// Taken from https://github.com/lvandeve/lodepng/blob/master/examples/example_encode_type.cpp
	// we're going to encode with a state rather than a convenient function, because enforcing a color type requires setting options
	lodepng::State state;
	// input color type
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	// output color type
	state.info_png.color.colortype = LCT_RGBA;
	state.info_png.color.bitdepth = 8;
	state.encoder.auto_convert = LAC_NO; // without this, it would ignore the output color type specified above and choose an optimal one instead

	unsigned error = lodepng::encode(png, image, width, height, state);
	if (!error) lodepng::save_file(png, file_name);

	//if there's an error, display it
	if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	printing_string = "Done Saving: " + file_name;
	Log(printing_string.c_str());
	free(image);
 }

void bonobo::screenShot(std::string file_name, const glm::vec2 &lower_corner, const glm::vec2 &upper_corner, const glm::vec2 &windows_size)
{

	auto const relative_to_absolute = [](float coord, int size) {
		return static_cast<GLint>((coord + 1.0f) / 2.0f * size);
	};
	auto const viewport_origin = glm::ivec2(relative_to_absolute(lower_corner.x, windows_size.x),
		relative_to_absolute(lower_corner.y, windows_size.y));
	auto const viewport_size = glm::ivec2(relative_to_absolute(upper_corner.x, windows_size.x),
		relative_to_absolute(upper_corner.y, windows_size.y))
		- viewport_origin;
	glFinish();
	const unsigned int image_size = viewport_size.x * viewport_size.y * 4; // 4 channels

	unsigned char* image = (unsigned char*)malloc(image_size);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(viewport_origin.x, viewport_origin.y, viewport_size.x, viewport_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);

	std::thread t = std::thread(threadSaveScreenshot, file_name, viewport_size.x, viewport_size.y, image);
	t.detach();
}

void bonobo::saveConfig(std::string file_name, const bool &using_fxaa,
	const bool &using_sobel, FPSCameraf &camera,
	const float &k_feedback_min, const float &k_feedback_max,
	const int &sample_amount, const float &accumulation_jitter_spread,
	const glm::vec2 &lower_corner, const glm::vec2 &upper_corner,
	const bool &ghosting_test, const int &ghosting_test_amount)
{
	std::ofstream log_file;

	if (ghosting_test)
	{
		log_file.open(config::resources_path(file_name + "_log_gt.txt"));
		log_file << ghosting_test_amount << std::endl;
		log_file.close();
	}

	file_name += "_log.txt";
	log_file.open(config::resources_path(file_name));
	log_file << "Scene Information" << std::endl;
	log_file << "Using FXAA: " << using_fxaa << std::endl;
	log_file << "Using Sobel: " << using_sobel << std::endl;
	log_file << "Jittering Camera?: " << camera.jitterProjection << std::endl;
	log_file << "Jitter Spread: " << camera.jitterSpread << std::endl;
	log_file << "k_feedback_min: " << k_feedback_min << std::endl;
	log_file << "k_feedback_max: " << k_feedback_max << std::endl;
	log_file << "Camera: " << std::endl;
	log_file << camera << std::endl;
	log_file << "Saving Information" << std::endl;
	log_file << "Sample Amount: " << sample_amount << std::endl;
	log_file << "Accumulation Buffer Jitter Spread: " << accumulation_jitter_spread << std::endl;
	log_file << "Lower Corner: " << std::endl << lower_corner << std::endl;
	log_file << "Upper Corner: " << std::endl << upper_corner << std::endl;
	log_file.close();
}

void bonobo::beginTimeQuery(GLuint &query)
{
	if (query == 0u)
	{
		glGenQueries(1, &query);
	}
	glBeginQuery(GL_TIME_ELAPSED, query);
}

void bonobo::endTimeQuery(const GLuint &query)
{
	glEndQuery(GL_TIME_ELAPSED);
}

void bonobo::collectTimeQuery(const GLuint & query, double & time, const bool wait_results)
{
	GLint done = 0;
	while (wait_results && !done) {
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
	}
	GLuint64 elapsed_time;
	glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
	time = static_cast<double>(elapsed_time) / 1000000.0; // convert from nanoseconds
}

void bonobo::destroyTimeQuery(GLuint & query)
{
	glDeleteQueries(1, &query);
	query = 0u;
}
