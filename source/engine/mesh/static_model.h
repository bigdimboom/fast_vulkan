#pragma once
#include "basic_mesh.h"
#include "image.h"
#include <string>
#include <memory>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace mesh
{

class StaticModel
{
public:
	StaticModel(const std::string& path, bool gamma = false);
	~StaticModel();

	StaticModel(const StaticModel&) = delete;
	StaticModel(StaticModel&&) = delete;
	void operator=(const StaticModel&) = delete;
	void operator=(StaticModel&&) = delete;

	const std::vector<std::shared_ptr<BasicMesh>>& meshes() const;
	const std::vector<std::shared_ptr<Image2D>>& textures() const;
	bool gammaCorrection() const;

private:
	std::vector<std::shared_ptr<BasicMesh>> d_meshes;
	std::vector<std::shared_ptr<Image2D>> d_textures;
	bool d_gammaCorrection = false;

	// VAR HELPERS
	std::string d_model_dir;
	std::map<std::string, std::shared_ptr<Image2D>> d_added;

	// HELPERS
	void loadModel(const std::string& path);
	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene);
	void processMesh(aiMesh* mesh, const aiScene* scene, BasicMesh& out);
};

} // end namespace mesh