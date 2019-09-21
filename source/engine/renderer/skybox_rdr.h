#pragma once
#include "irenderer.h"
#include "../vkapi/vk_ctx.h"
#include "../camera/free_camera.h"
#include <string>
#include <vector>

namespace renderer
{

class SkyboxRdr : public IRenderer
{
public:
	enum Face
	{
		Front = 0,
		Back,
		Top,
		Bottom,
		Right,
		Left,
		FaceSize
	};

	SkyboxRdr(std::shared_ptr<vkapi::Context> vkCtx,
		std::shared_ptr<camera::FreeCamera> cam,
		const std::vector<std::string>& faces_image_files);

	~SkyboxRdr();

	SkyboxRdr(const SkyboxRdr&) = delete;
	SkyboxRdr(SkyboxRdr&&) = delete;
	void operator=(const SkyboxRdr&) = delete;
	void operator=(SkyboxRdr&&) = delete;

	// Inherited via IRenderer
	void render() override;

	void setViewport(vk::Viewport viewport);
	void setRenderArea(vk::Rect2D scissor);

private:

	std::shared_ptr<vkapi::Context> d_vkCtx;
	std::shared_ptr<camera::FreeCamera> d_cam;
	vk::Viewport d_viewport;
	vk::Rect2D d_renderArea;

	struct MVP
	{
		glm::mat4 view;
		glm::mat4 proj;
	}d_mvp;

	struct
	{
		uint32_t nVerts;
		std::shared_ptr<vkapi::BufferObject> vbo; // points
		vk::PipelineVertexInputStateCreateInfo inputState;
		vk::VertexInputBindingDescription inputBinding;
		std::vector<vk::VertexInputAttributeDescription> inputAttributes;
	}d_bufferData;

	struct
	{
		std::shared_ptr<vkapi::BufferObject> mvp_buf = nullptr;

		vk::Format imageFormat;
		std::shared_ptr<vkapi::ImageObject>  cubemap = nullptr;
		vk::ImageView cubemapView;
		vk::Sampler cubemapSampler;

		std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {};
		vk::DescriptorSetLayout descriptorSetLayout = {};
		vk::DescriptorSet descriptorSet = {};
		vk::PipelineLayout pipelineLayout = {};

		vk::DescriptorBufferInfo descriptorBufferInfo = {};
		vk::DescriptorImageInfo descriptorImageInfo = {};
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {};

	}d_ubo;

	struct
	{
		vk::Pipeline pipeline;
		vk::PipelineCache pipelineCache;
		vk::ShaderModule vs;
		vk::ShaderModule fs;
		std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfos;
	}d_pipeline;

	// helpers
	bool loadCubeMap(const std::vector<std::string>& face_files);
	void setupVBO();
	void setupUBO();
	void setupPipeline();
};


} // end namespace renderer