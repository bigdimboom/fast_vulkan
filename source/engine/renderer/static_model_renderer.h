#pragma once
#include "irenderer.h"
#include "../vkapi/vk_ctx.h"
#include "../camera/free_camera.h"
#include "../mesh/basic_mesh.h"
#include "../mesh/static_model.h"
#include "../octree/linear_octree.h"

#include <string>
#include <glm/glm.hpp>

namespace renderer
{

class StaticModelRenderer : public IRenderer
{
public:
	StaticModelRenderer(std::shared_ptr<vkapi::Context> vkCtx, std::shared_ptr<camera::FreeCamera> camera);
	~StaticModelRenderer();

	void setCamera(std::shared_ptr<camera::FreeCamera> cam);
	void setViewport(int x, int y, int width, int height);
	void setModel(std::shared_ptr<mesh::StaticModel> model, const glm::mat4& transform = glm::mat4(1.0f));
	bool build(bool clearhost = true);

	void render() override;

private:
	vk::Viewport d_viewport;
	vk::Rect2D d_renderArea;

	std::shared_ptr<vkapi::Context> d_vkCtx;
	std::shared_ptr<camera::FreeCamera> d_camera;

	struct
	{
		std::shared_ptr<mesh::StaticModel> smodel = nullptr;
		glm::mat4 transform = glm::mat4(1.0);
	}d_input;

	struct MVP
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	}d_mvp;

	struct BufferData // vbos
	{
		std::shared_ptr<vkapi::BufferObject> vbo;
		std::vector<size_t> offsets;
		vk::PipelineVertexInputStateCreateInfo inputState;
		vk::VertexInputBindingDescription inputBinding;
		std::vector<vk::VertexInputAttributeDescription> inputAttributes;
	}d_vertexInput;

	struct IndexBufferData
	{
		std::vector<std::shared_ptr<vkapi::BufferObject>> ibos;
		std::vector<size_t> index_count;
	}d_indexInput;

	struct UBO // unifroms
	{
		std::shared_ptr<vkapi::BufferObject> mvp_buffer;
		//std::vector<std::shared_ptr<vkapi::ImageObject>> textures;
		//std::vector<vk::ImageView> texture_views;
		//std::vector<vk::Sampler> texture_samplers;

		// TODO:
		std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
		vk::DescriptorSetLayout descriptorSetLayout = {};
		vk::DescriptorSet descriptorSet = {};
		vk::PipelineLayout pipelineLayout = {};
		vk::DescriptorBufferInfo mvp_buffer_info;

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
	}d_ubo;

	struct Pipeline
	{
		vk::Pipeline pipeline;
		vk::PipelineCache pipelineCache;
		vk::ShaderModule vs;
		vk::ShaderModule fs;
		std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfos;
	}d_pipeline;


	std::unique_ptr<octree::DrawMeshOctree> d_tree;

	// HELPERS
	bool buildVBO();
	void buildIBO();
	void buildUBO();
	void buildPipeline();
};

} // end namespace renderer