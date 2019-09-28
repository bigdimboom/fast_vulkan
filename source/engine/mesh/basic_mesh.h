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

struct Vertex
{
	Position point = { 0.0f, 0.0f, 0.0f };
	Normal normal = { 0.0f, 0.0f, 0.0f };
	Tangent tagent = { 0.0f, 0.0f, 0.0f };
	BiTangent bitangent = { 0.0f, 0.0f, 0.0f };
	Color color = { 0.0f, 0.0f, 0.0f, 1.0f };
	UV uv = { 0.0f, 0.0f };
};

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