#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace mesh
{

using Position = glm::vec3;
using UV = glm::vec2;
using Normal = glm::vec3;
using Tangent = glm::vec3;
using BiTangent = glm::vec3;
using Color = glm::vec4;

struct BasicMesh
{
	BasicMesh();
	~BasicMesh();

	std::vector<Position> positions;
	std::vector<UV> texcoords;
	std::vector<Normal> normals;
	std::vector<Tangent> tangents;
	std::vector<BiTangent> bitangents;
	std::vector<Color> colors;
	std::vector<uint32_t> indices;
};

} // end namespace mesh