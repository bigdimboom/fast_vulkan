#include "util.h"
#include <SDL2/SDL.h>
#include <assert.h>

namespace mesh
{




Utility::Utility()
{
}

Utility::~Utility()
{
}

bool Utility::computeNormals(BasicMesh& mesh, bool force_replace_old_normals)
{
	if (mesh.indices.empty() || mesh.positions.empty())
	{
		SDL_Log("invalid mesh input, postions or indices are empty");
		return false;
	}

	if (mesh.normals.empty() == false || force_replace_old_normals == false)
	{
		SDL_Log("mesh contains non empty normals skip this operations");
		return false;
	}

	std::vector<uint32_t> count;
	mesh.normals.clear();
	mesh.normals.resize(mesh.positions.size());
	count.resize(mesh.positions.size(), 0);

	for (size_t i = 0; i < mesh.normals.size(); i += 3)
	{
		uint32_t i0 = mesh.indices[i];
		uint32_t i1 = mesh.indices[i + 1];
		uint32_t i2 = mesh.indices[i + 2];

		auto vec1 = mesh.positions[i1] - mesh.positions[i0];
		auto vec2 = mesh.positions[i2] - mesh.positions[i0];
		auto normal = glm::cross(vec1, vec2);

		mesh.normals[i0] += normal;
		mesh.normals[i1] += normal;
		mesh.normals[i2] += normal;

		++count[i0];
		++count[i1];
		++count[i2];
	}

	for (size_t i = 0; i < mesh.normals.size(); ++i)
	{
		mesh.normals[i] /= count[i];
		mesh.normals[i] = glm::normalize(mesh.normals[i]);
	}

	return true;
}

bool Utility::createVertexArray(const BasicMesh& mesh, std::vector<Vertex>& output)
{
	if (mesh.indices.empty() || mesh.positions.empty())
	{
		SDL_Log("invalid mesh input, postions or indices are empty");
		return false;
	}

	output.clear();
	output.resize(mesh.positions.size());

	for (size_t i = 0; i < mesh.positions.size(); ++i)
	{
		Vertex v;
		v.point = mesh.positions[i];

		if (mesh.normals.size())
		{
			assert(mesh.normals.size() == mesh.positions.size());
			v.normal = mesh.normals[i];
		}

		if (mesh.colors.size())
		{
			assert(mesh.colors.size() == mesh.positions.size());
			v.color = mesh.colors[i];
		}

		if (mesh.tangents.size())
		{
			assert(mesh.tangents.size() == mesh.positions.size());
			v.tagent = mesh.tangents[i];
		}

		if (mesh.bitangents.size())
		{
			assert(mesh.bitangents.size() == mesh.positions.size());
			v.bitangent = mesh.bitangents[i];
		}

		if (mesh.texcoords.size())
		{
			assert(mesh.texcoords.size() == mesh.positions.size());
			v.uv = mesh.texcoords[i];
		}

		output[i] = v;
	}

	return true;
}

bool Utility::computeTangents(BasicMesh& mesh_input_output, bool force_replace_old_tangents)
{
	assert(0);
	throw "not implemented.";
	return false;
}

bool Utility::transformPointCloud(BasicMesh& mesh, const glm::mat4& matrix)
{
	if (mesh.indices.empty() || mesh.positions.empty())
	{
		SDL_Log("invalid mesh input, postions or indices are empty");
		return false;
	}

	for (auto& elem : mesh.positions)
	{
		elem = glm::vec3(matrix * glm::vec4(elem, 1.0f));
	}

	return true;
}

} // end namespace mesh
