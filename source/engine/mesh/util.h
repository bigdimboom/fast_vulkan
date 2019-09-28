#pragma once
#include "basic_mesh.h"

namespace mesh
{

class Utility
{
public:
	Utility();
	~Utility();

	static bool computeNormals(BasicMesh& mesh_input_output, bool force_replace_old_normals = false);
	static bool createVertexArray(const BasicMesh& mesh_input, std::vector<Vertex>& output);
	static bool computeTangents(BasicMesh& mesh_input_output, bool force_replace_old_tangents = false);
	static bool transformPointCloud(BasicMesh& mesh_input_output, const glm::mat4& matrix);

};

} // end namespace mesh