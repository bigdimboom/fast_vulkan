#include "skinned_mesh.h"

namespace mesh
{

SkinnedMesh::SkinnedMesh()
{
}

SkinnedMesh::SkinnedMesh(const BasicMesh& src_mesh)
{
}

SkinnedMesh::~SkinnedMesh()
{
}

uint32_t SkinnedMesh::triangle_index_count() const
{
	return static_cast<uint32_t>(triangle_indices.size());
}

uint32_t SkinnedMesh::vertex_count() const
{
	uint32_t vertex_count = 0;
	for (size_t i = 0; i < parts.size(); ++i) {
		vertex_count += parts[i].vertex_count();
	}
	return vertex_count;
}

uint32_t SkinnedMesh::max_influences_count() const
{
	uint32_t max_influences_count = 0;
	for (size_t i = 0; i < parts.size(); ++i) {
		const uint32_t influences_count = parts[i].influences_count();
		max_influences_count = influences_count > max_influences_count
			? influences_count
			: max_influences_count;
	}
	return max_influences_count;
}

bool SkinnedMesh::skinned() const
{
	for (size_t i = 0; i < parts.size(); ++i) {
		if (parts[i].influences_count() != 0) {
			return true;
		}
	}
	return false;
}

uint32_t SkinnedMesh::num_joints() const
{
	return static_cast<uint32_t>(inverse_bind_poses.size());
}

uint32_t SkinnedMesh::highest_joint_index() const
{
	// Takes advantage that joint_remaps is sorted.
	return joint_remaps.size() != 0 ? static_cast<uint32_t>(joint_remaps.back()) : 0;
}

uint32_t Part::vertex_count() const
{
	return static_cast<uint32_t>(positions.size());
}

uint32_t Part::influences_count() const
{
	const auto vcount = vertex_count();
	if (vcount == 0) {
		return 0;
	}
	return static_cast<uint32_t>(joint_indices.size()) / vcount;
}
} // end namespace mesh