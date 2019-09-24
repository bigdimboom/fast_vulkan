#pragma once
#include "basic_mesh.h"

namespace mesh
{

struct Part
{
	std::vector<Position> positions;
	std::vector<UV> texcoords;
	std::vector<Normal> normals;
	std::vector<Tangent> tangents;
	std::vector<BiTangent> bitangent;
	std::vector<Color> colors;
	std::vector<uint16_t>  joint_indices;  // Stride equals influences_count
	std::vector<float>  joint_weights;  // Stride equals influences_count - 1

	uint32_t vertex_count() const;
	uint32_t influences_count() const;
};

using Parts = std::vector<Part>;
using TriangleIndices = std::vector<uint32_t>;
using JointRemaps = std::vector<uint32_t>;
using InversBindPoses = std::vector<glm::mat4>;

struct SkinnedMesh
{
	SkinnedMesh();
	SkinnedMesh(const BasicMesh& src_mesh);
	~SkinnedMesh();

	// Defines a portion of the mesh. A mesh is subdivided in sets of vertices
	// with the same number of joint influences.
	Parts parts;

	// Triangles indices. Indices are shared across all parts.
	TriangleIndices triangle_indices;

	// Joints remapping indices. As a skin might be influenced by a part of the
	// skeleton only, joint indices and inverse bind pose matrices are reordered
	// to contain only used ones. Note that this array is sorted.
	JointRemaps joint_remaps;

	// Inverse bind-pose matrices. These are only available for skinned meshes.
	InversBindPoses inverse_bind_poses;

	// Number of triangle indices for the mesh.
	uint32_t triangle_index_count() const;

	// Number of vertices for all mesh parts.
	uint32_t vertex_count() const;

	// Maximum number of joints influences for all mesh parts.
	uint32_t max_influences_count() const;

	// Test if the mesh has skinning informations.
	bool skinned() const;

	// Returns the number of joints used to skin the mesh.
	uint32_t num_joints() const;

	// Returns the highest joint number used in the skeleton.
	uint32_t highest_joint_index() const;
};

} // end namespace mesh