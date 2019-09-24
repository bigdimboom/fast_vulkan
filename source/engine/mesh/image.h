#pragma once
#include <vector>
#include <assimp/material.h>

namespace mesh
{

enum ImageKind
{
	None = aiTextureType_NONE,
	DiffuseMap = aiTextureType_DIFFUSE,
	SpecularMap = aiTextureType_SPECULAR,
	AmbientMap = aiTextureType_AMBIENT,
	EmissiveMap = aiTextureType_EMISSIVE,
	HeightMap = aiTextureType_HEIGHT,
	NormalsMap = aiTextureType_NORMALS,
	ShininessMap = aiTextureType_SHININESS,
	OpacityMap = aiTextureType_OPACITY,
	DisplacementMap = aiTextureType_DISPLACEMENT,
	LightMap = aiTextureType_LIGHTMAP,
	ReflectionMap = aiTextureType_REFLECTION,
	UnknownMap = aiTextureType_UNKNOWN
};

struct Image2D
{
	int width;
	int height;
	int channels;
	int elem_size;
	std::vector<uint8_t> buffer;
	ImageKind kind;
};

} // end namespace mesh