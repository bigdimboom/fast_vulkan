#include "static_model.h"
#include <SDL2/SDL.h>
#include <assert.h>

#include "../util/image_utils.h"

#include <iostream>

namespace mesh
{

StaticModel::StaticModel(const std::string& path, bool gamma)
{
	loadModel(path);
}

StaticModel::~StaticModel()
{
}

const std::vector<std::shared_ptr<BasicMesh>>& StaticModel::meshes() const
{
	return d_meshes;
}

const std::vector<std::shared_ptr<Image2D>>& StaticModel::textures() const
{
	return d_textures;
}

bool StaticModel::gammaCorrection() const
{
	return false;
}

void StaticModel::loadModel(const std::string& path)
{
	// read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		SDL_Log("Error:ASSIMP:: %s", importer.GetErrorString());
		return;
	}
	// retrieve the directory path of the filepath
	d_model_dir = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);

	// material retrieval
	for (const auto& elem : d_added)
	{
		d_textures.push_back(elem.second);
	}
	d_added.clear();
}

void StaticModel::processNode(aiNode* node, const aiScene* scene)
{
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		d_meshes.push_back(std::make_shared<BasicMesh>());
		processMesh(mesh, scene, *d_meshes.back());
	}

	// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

void StaticModel::processMesh(aiMesh* mesh, const aiScene* scene, BasicMesh& out)
{
	///////////////////////
	/// process vertices///
	///////////////////////

	if (mesh->HasPositions())
	{
		out.positions.resize(mesh->mNumVertices);
		memcpy(out.positions.data(), mesh->mVertices, sizeof(aiVector3D) * mesh->mNumVertices);
	}

	if (mesh->HasNormals())
	{
		out.normals.resize(mesh->mNumVertices);
		memcpy(out.normals.data(), mesh->mNormals, sizeof(aiVector3D) * mesh->mNumVertices);
	}

	if (mesh->GetNumUVChannels() && mesh->HasTextureCoords(0))
	{
		out.texcoords.resize(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			out.texcoords[i].x = mesh->mTextureCoords[0][i].x;
			out.texcoords[i].y = mesh->mTextureCoords[0][i].y;
		}
	}

	if (mesh->GetNumColorChannels() && mesh->HasVertexColors(0))
	{
		out.colors.resize(mesh->mNumVertices);
		memcpy(out.colors.data(), mesh->mColors[0], sizeof(aiColor4D) * mesh->mNumVertices);
	}

	if (mesh->HasTangentsAndBitangents())
	{
		out.tangents.resize(mesh->mNumVertices);
		out.bitangents.resize(mesh->mNumVertices);
		memcpy(out.tangents.data(), mesh->mTangents, sizeof(aiVector3D) * mesh->mNumVertices);
		memcpy(out.bitangents.data(), mesh->mBitangents, sizeof(aiVector3D) * mesh->mNumVertices);
	}

	if (mesh->HasFaces())
	{
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector

			if (face.mNumIndices != 3)
			{
				SDL_Log("Error occours. This program only supports triangle mesh");
				exit(EXIT_FAILURE);
			}

			for (unsigned int j = 0; j < face.mNumIndices; ++j)
			{
				out.indices.push_back(face.mIndices[j]);
			}
		}
	}

	///////////////////////
	// process materials///
	///////////////////////

	// process materials
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
	// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
	// Same applies to other texture as the following list summarizes:
	// diffuse: texture_diffuseN
	// specular: texture_specularN
	// normal: texture_normalN

	for (int i = aiTextureType_NONE; i <= aiTextureType_UNKNOWN; ++i)
	{
		aiTextureType type = (aiTextureType)i;

		int count = material->GetTextureCount(type);

		for (int j = 0; j < count; ++j)
		{
			aiString str;
			material->GetTexture(type, j, &str);

			std::string filename(str.C_Str());
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
			std::string full_path = d_model_dir + "/" + filename;

			if (d_added.count(filename) > 0)
			{
				continue;
			}

			auto img = std::make_shared<Image2D>();

			if (util::ImageUtility::load(full_path, img->buffer, img->width, img->height, img->channels))
			{
				img->elem_size = 1;
				img->kind = (ImageKind)i;
				d_added[filename] = img;
			}
		}
	}
}

} // end namespace mesh