#include "parametric_shapes.hpp"
#include "core/Log.h"
#include "core/utils.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

bonobo::mesh_data
parametric_shapes::createQuad(float width, float height,
		unsigned int res_width, unsigned int res_height)
{
	auto const vertices_nb = res_width * res_height;

		auto vertices = std::vector<glm::vec3>(vertices_nb);
		auto normals = std::vector<glm::vec3>(vertices_nb);
		auto texcoords = std::vector<glm::vec3>(vertices_nb);
		auto tangents = std::vector<glm::vec3>(vertices_nb);
		auto binormals = std::vector<glm::vec3>(vertices_nb);

		float _height = 0.0f,                                                       // 'stepping'-variable for height: will go 0 - H
			dheight =  height/ (static_cast<float>(res_height) - 1.0f); // step size, depending on the resolution

		float _width = 0.0f,                                             // 'stepping'-variable for width: will go 0 - W
			dwidth= width / (static_cast<float>(res_width) - 1.0f); // step size, depending on the resolution

																		 // generate vertices iteratively
		size_t index = 0u;
		for (unsigned int i = 0u; i < res_height ; ++i) {

			_width = 0.0f;
			for (unsigned int j = 0u; j < res_width; ++j) {
				// vertex
				vertices[index] = glm::vec3(_width,
					0.0f,
					_height);

				// texture coordinates
				texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_width) - 1.0f),
					static_cast<float>(i) / (static_cast<float>(res_height) - 1.0f),
					0.0f);

				// tangent
				auto t = glm::vec3(1.0f, 0.0f, 0.0f);
				t = glm::normalize(t);
				tangents[index] = t;

				// binormal
				auto b = glm::vec3(0.0f, 0.0f, 1.0f);
				b = glm::normalize(b);
				binormals[index] = b;

				// normal
				auto const n = glm::vec3(0.0f, 1.0f, 0.0f);
				normals[index] = n;

				_width += dwidth;
				++index;
			}

			_height += dheight;
		}

		// create index array
		auto indices = std::vector<glm::uvec3>(2u * (res_height - 1u) * (res_width - 1u));

		// generate indices iteratively
		index = 0u;
		for (unsigned int i = 0u; i < res_width - 1u; ++i)
		{
			for (unsigned int j = 0u; j < res_height - 1u; ++j)
			{
				indices[index] = glm::uvec3(res_height * i + j,
					res_height * i + j + 1u,
					res_height * i + j + res_height);
				++index;

				indices[index] = glm::uvec3(res_height * i + j + 1u,
					res_height * i + j + res_height + 1u,
					res_height * i + j + res_height);
				++index;
			}
		}

		bonobo::mesh_data data;
		glGenVertexArrays(1, &data.vao);
		assert(data.vao != 0u);
		glBindVertexArray(data.vao);

		auto const vertices_offset = 0u;
		auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
		auto const normals_offset = vertices_size;
		auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
		auto const texcoords_offset = normals_offset + normals_size;
		auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
		auto const tangents_offset = texcoords_offset + texcoords_size;
		auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
		auto const binormals_offset = tangents_offset + tangents_size;
		auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
		auto const bo_size = static_cast<GLsizeiptr>(vertices_size
			+ normals_size
			+ texcoords_size
			+ tangents_size
			+ binormals_size
			);
		glGenBuffers(1, &data.bo);
		assert(data.bo != 0u);
		glBindBuffer(GL_ARRAY_BUFFER, data.bo);
		glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

		glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

		glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

		glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

		glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
		glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
		glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

		glBindBuffer(GL_ARRAY_BUFFER, 0u);

		data.indices_nb = indices.size() * 3u;
		glGenBuffers(1, &data.ibo);
		assert(data.ibo != 0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

		glBindVertexArray(0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

		return data;

}

bonobo::mesh_data
parametric_shapes::createQuad(unsigned int width, unsigned int height)
{
	//! \todo Fill in the blanks
	auto const vertices = std::array<glm::vec3, 4>{
		glm::vec3(0.0f,                      0.0f,                       0.0f),
		glm::vec3(static_cast<float>(width), 0.0f,                       0.0f),
		glm::vec3(static_cast<float>(width), static_cast<float>(height), 0.0f),
		glm::vec3(0.0f,                      static_cast<float>(height), 0.0f)
	};

	auto const indices = std::array<glm::uvec3, 2>{
		glm::uvec3(0u, 1u, 2u),
		glm::uvec3(0u, 2u, 3u)
	};

	bonobo::mesh_data data;

	//
	// NOTE:
	//
	// Only the values preceeded by a `\todo` tag should be changed, the
	// other ones are correct!
	//

	// Create a Vertex Array Object: it will remember where we stored the
	// data on the GPU, and  which part corresponds to the vertices, which
	// one for the normals, etc.
	//
	// The following function will create new Vertex Arrays, and pass their
	// name in the given array (second argument). Since we only need one,
	// pass a pointer to `data.vao`.
	glGenVertexArrays(1, &data.vao);

	// To be able to store information, the Vertex Array has to be bound
	// first.
	glBindVertexArray(data.vao);

	// To store the data, we need to allocate buffers on the GPU. Let's
	// allocate a first one for the vertices.
	//
	// The following function's syntax is similar to `glGenVertexArray()`:
	// it will create multiple OpenGL objects, in this case buffers, and
	// return their names in an array. Have the buffer's name stored into
	// `data.bo`.
	glGenBuffers(1, &data.bo);

	// Similar to the Vertex Array, we need to bind it first before storing
	// anything in it. The data stored in it can be interpreted in
	// different ways. Here, we will say that it is just a simple 1D-array
	// and therefore bind the buffer to the corresponding target.
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);

	glBufferData(GL_ARRAY_BUFFER, /*! \todo how many bytes should the buffer contain? */vertices.size() * sizeof(vertices[0]),
	             /* where is the data stored on the CPU? */vertices.data(),
	             /* inform OpenGL that the data is modified once, but used often */GL_STATIC_DRAW);

	// Vertices have been just stored into a buffer, but we still need to
	// tell Vertex Array where to find them, and how to interpret the data
	// within that buffer.
	//
	// You will see shaders in more detail in lab 3, but for now they are
	// just pieces of code running on the GPU and responsible for moving
	// all the vertices to clip space, and assigning a colour to each pixel
	// covered by geometry.
	// Those shaders have inputs, some of them are the data we just stored
	// in a buffer object. We need to tell the Vertex Array which inputs
	// are enabled, and this is done by the following line of code, which
	// enables the input for vertices:
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));

	// Once an input is enabled, we need to explain where the data comes
	// from, and how it interpret it. When calling the following function,
	// the Vertex Array will automatically use the current buffer bound to
	// GL_ARRAY_BUFFER as its source for the data. How to interpret it is
	// specified below:
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices),
	                      /*! \todo how many components do our vertices have? */3,
	                      /* what is the type of each component? */GL_FLOAT,
	                      /* should it automatically normalise the values stored */GL_FALSE,
	                      /* once all components of a vertex have been read, how far away (in bytes) is the next vertex? */0,
	                      /* how far away (in bytes) from the start of the buffer is the first vertex? */reinterpret_cast<GLvoid const*>(0x0));

	// Now, let's allocate a second one for the indices.
	//
	// Have the buffer's name stored into `data.ibo`.
	glGenBuffers(1, /*! \todo fill me */&data.ibo);

	// We still want a 1D-array, but this time it should be a 1D-array of
	// elements, aka. indices!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, /*! \todo bind the previously generated Buffer */data.ibo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, /*! \todo how many bytes should the buffer contain? */static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)),
	             /* where is the data stored on the CPU? */indices.data(),
	             /* inform OpenGL that the data is modified once, but used often */GL_STATIC_DRAW);

	data.indices_nb = /*! \todo how many indices do we have? */indices.size() * 3u;

	// All the data has been recorded, we can unbind them.
	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createSphere(unsigned int const res_theta,
                                unsigned int const res_phi, float const radius)
{

	auto const vertices_nb = res_phi * res_theta;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float theta = 0.0f,                                                       // 'stepping'-variable for theta: will go 0 - 2PI
		dtheta = 2.0f * bonobo::pi / (static_cast<float>(res_theta) - 1.0f); // step size, depending on the resolution

	float phi = 0.0f,                                             // 'stepping'-variable for phi: will go 0 - PI
		dphi = bonobo::pi / (static_cast<float>(res_phi) - 1.0f); // step size, depending on the resolution

																	 // generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_phi ; ++i) {
		
		float cos_phi = std::cos(phi),
			sin_phi = std::sin(phi);

		theta = 0.0f;
		for (unsigned int j = 0u; j < res_theta; ++j) {
			float cos_theta = std::cos(theta),
				sin_theta = std::sin(theta);
			// vertex
			vertices[index] = glm::vec3(radius * sin_theta * sin_phi,
				-radius * cos_phi,
				radius * cos_theta * sin_phi);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_theta) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_phi) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(cos_theta, 0.0f, -sin_theta);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(sin_theta * cos_phi, sin_phi, cos_theta * cos_phi);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			theta += dtheta;
			++index;
		}

		phi += dphi;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * (res_theta - 1u) * (res_phi - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < res_phi - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_theta - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_theta * i + j,
				res_theta * i + j + 1u,
				res_theta * i + j + res_theta);
			++index;

			indices[index] = glm::uvec3(res_theta * i + j + 1u,
				res_theta * i + j + res_theta + 1u,
				res_theta * i + j + res_theta);
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		+ tangents_size
		+ binormals_size
		);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createTorus(unsigned int const res_theta,
                               unsigned int const res_phi, float const rA,
                               float const rB)
{
	auto const vertices_nb = res_phi * res_theta;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float theta = 0.0f,                                                       // 'stepping'-variable for theta: will go 0 - 2PI
		dtheta = 2.0f * bonobo::pi / (static_cast<float>(res_theta) - 1.0f); // step size, depending on the resolution

	float phi = 0.0f,                                             // 'stepping'-variable for phi: will go 0 - 2PI
		dphi = 2.0f * bonobo::pi / (static_cast<float>(res_phi) - 1.0f); // step size, depending on the resolution

																  // generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_phi; ++i) {

		float cos_phi = std::cos(phi),
			sin_phi = std::sin(phi);

		theta = 0.0f;
		for (unsigned int j = 0u; j < res_theta; ++j) {
			float cos_theta = std::cos(theta),
				sin_theta = std::sin(theta),
				r_sum = rA + (rB * cos_theta);
			// vertex
			vertices[index] = glm::vec3(r_sum * cos_phi,
				r_sum * sin_phi,
				-rB * sin_theta);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_theta) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_phi) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(-sin_theta * cos_phi, -sin_theta * sin_phi, -cos_theta);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(-sin_phi, cos_phi, 0.0f);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			theta += dtheta;
			++index;
		}

		phi += dphi;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * (res_theta - 1u) * (res_phi - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < res_phi - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_theta - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_theta * i + j,
				res_theta * i + j + 1u,
				res_theta * i + j + res_theta);
			++index;

			indices[index] = glm::uvec3(res_theta * i + j + 1u,
				res_theta * i + j + res_theta + 1u,
				res_theta * i + j + res_theta);
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		+ tangents_size
		+ binormals_size
		);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createCircleRing(unsigned int const res_radius,
                                    unsigned int const res_theta,
                                    float const inner_radius,
                                    float const outer_radius)
{
	auto const vertices_nb = res_radius * res_theta;

	auto vertices  = std::vector<glm::vec3>(vertices_nb);
	auto normals   = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents  = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float theta = 0.0f,                                                        // 'stepping'-variable for theta: will go 0 - 2PI
	      dtheta = 2.0f * bonobo::pi / (static_cast<float>(res_theta) - 1.0f); // step size, depending on the resolution

	float radius = 0.0f,                                                                     // 'stepping'-variable for radius: will go inner_radius - outer_radius
	      dradius = (outer_radius - inner_radius) / (static_cast<float>(res_radius) - 1.0f); // step size, depending on the resolution

	// generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_theta; ++i) {
		float cos_theta = std::cos(theta),
		      sin_theta = std::sin(theta);

		radius = inner_radius;

		for (unsigned int j = 0u; j < res_radius; ++j) {
			// vertex
			vertices[index] = glm::vec3(radius * cos_theta,
			                            radius * sin_theta,
			                            0.0f);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_radius) - 1.0f),
			                             static_cast<float>(i) / (static_cast<float>(res_theta)  - 1.0f),
			                             0.0f);

			// tangent
			auto t = glm::vec3(cos_theta, sin_theta, 0.0f);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(-sin_theta, cos_theta, 0.0f);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			radius += dradius;
			++index;
		}

		theta += dtheta;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * (res_theta - 1u) * (res_radius - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < res_theta - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_radius - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_radius * i + j,
			                            res_radius * i + j + 1u,
			                            res_radius * i + j + 1u + res_radius);
			++index;

			indices[index] = glm::uvec3(res_radius * i + j,
			                            res_radius * i + j + res_radius + 1u,
			                            res_radius * i + j + res_radius);
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
	                                            +normals_size
	                                            +texcoords_size
	                                            +tangents_size
	                                            +binormals_size
	                                            );
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}


bonobo::mesh_data
parametric_shapes::createMobiusStrip(unsigned int const res_t,
	unsigned int const res_s, float const r, float w)
{

	auto const vertices_nb = 2u * res_s * res_t;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float t = 0.0f,                                                       // 'stepping'-variable for t: will go 0 - 2PI
		dt = 2.0f * bonobo::pi / (static_cast<float>(res_t) - 1.0f); // step size, depending on the resolution

	float s = -w,                                             // 'stepping'-variable for s: will go -w - w
		ds = 2.0f * w / (static_cast<float>(res_s) - 1.0f); // step size, depending on the resolution

																  // generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_t; ++i) {

		float cos_t = std::cos(t),
			sin_t = std::sin(t),
			cos_half_t = std::cos(t / 2.0f),
			sin_half_t = std::sin(t / 2.0f);

		s = -w;
		for (unsigned int j = 0u; j < res_s; ++j) {
			float s_cos_half_t = s * cos_half_t,
				s_sin_half_t = s * sin_half_t;
			// vertex
			vertices[index] = glm::vec3((r + s * cos_half_t) * cos_t,
				(r + s * cos_half_t) * sin_t,
				s_sin_half_t);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_t) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_s) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(cos_half_t * cos_t, cos_half_t * sin_t, sin_half_t);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(-r * sin_t - s_cos_half_t * sin_t - s_sin_half_t * cos_t * 0.5f,
				r * cos_t + s_sin_half_t * cos_t - s_sin_half_t * sin_t * 0.5f,
				s * 0.5f * cos_half_t);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			s += ds;
			++index;
		}

		t += dt;
	}

	t = 0.0f;
	for (unsigned int i = 0u; i < res_t; ++i) {

		float cos_t = std::cos(t),
			sin_t = std::sin(t),
			cos_half_t = std::cos(t / 2.0f),
			sin_half_t = std::sin(t / 2.0f);

		s = -w;
		for (unsigned int j = 0u; j < res_s; ++j) {
			float s_cos_half_t = s * cos_half_t,
				s_sin_half_t = s * sin_half_t;
			// vertex
			vertices[index] = glm::vec3((r + s * cos_half_t) * cos_t,
				(r + s * cos_half_t) * sin_t,
				s_sin_half_t);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_t) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_s) - 1.0f),
				0.0f);

			// tangent
			auto t = - glm::vec3(cos_half_t * cos_t, cos_half_t * sin_t, sin_half_t);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(-r * sin_t - s_cos_half_t * sin_t - s_sin_half_t * cos_t * 0.5f,
				r * cos_t + s_sin_half_t * cos_t - s_sin_half_t * sin_t * 0.5f,
				s * 0.5f * cos_half_t);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			s += ds;
			++index;
		}

		t += dt;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * 2u * (res_t - 1u) * (res_s - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int j = 0u; j < res_s - 1u; ++j)
	{
		for (unsigned int i = 0u; i < res_t - 1u; ++i)
		{
			indices[index] = glm::uvec3(res_t * i + j,
				res_t * i + j + 1u,
				res_t * i + j + res_t);
			++index;

			indices[index] = glm::uvec3(res_t * i + j + 1u,
				res_t * i + j + res_t + 1u,
				res_t * i + j + res_t);
			++index;
		}
	}

	unsigned int face_size = res_s * res_t;

	for (unsigned int j = 0u; j < res_s - 1u; ++j)
	{
		for (unsigned int i = 0u; i < res_t - 1u; ++i)
		{
			indices[index] = glm::uvec3(res_t * i + j + res_t + face_size,
				res_t * i + j + 1u + face_size,
				res_t * i + j + face_size);
			++index;

			indices[index] = glm::uvec3(res_t * i + j + res_t + face_size,
				res_t * i + j + res_t + 1u + face_size,
				res_t * i + j + 1u + face_size);
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		+ tangents_size
		+ binormals_size
		);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}
