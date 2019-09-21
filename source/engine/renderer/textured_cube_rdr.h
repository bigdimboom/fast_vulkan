#pragma once
#include "irenderer.h"
#include "../vkapi/vk_ctx.h"
#include "../camera/free_camera.h"

namespace renderer
{

class TexturedCubeRdr : public renderer::IRenderer
{
public:
	TexturedCubeRdr(std::shared_ptr<vkapi::Context> vkCtx, std::shared_ptr<camera::FreeCamera> cam, const char* file = nullptr);
	~TexturedCubeRdr();

	TexturedCubeRdr(const TexturedCubeRdr&) = delete;
	TexturedCubeRdr(TexturedCubeRdr&&) = delete;
	void operator=(const TexturedCubeRdr&) = delete;
	void operator=(TexturedCubeRdr&&) = delete;

	void setTransform(const glm::mat4& xfm);

	void setCamera(std::shared_ptr<camera::FreeCamera> cam = nullptr);

	void render() override;

	void setViewport(vk::Viewport viewport);
	void setRenderAera(vk::Rect2D area);

private:

	std::shared_ptr<vkapi::Context> d_vkCtx;
	std::shared_ptr<camera::FreeCamera> d_cam;
	vk::Viewport d_viewport;
	vk::Rect2D   d_renderArea;

	struct
	{
		uint32_t nVerts;
		std::shared_ptr<vkapi::BufferObject> vbo; // points, uvs
		vk::PipelineVertexInputStateCreateInfo inputState;
		vk::VertexInputBindingDescription inputBinding;
		std::vector<vk::VertexInputAttributeDescription> inputAttributes;
	}d_bufferData;

	struct MVP
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	}d_mvp;

	struct
	{
		std::shared_ptr<vkapi::BufferObject> mvpBuffer = nullptr;
		std::shared_ptr<vkapi::ImageObject> texture2D = nullptr;
		vk::ImageView textureView = {};
		vk::Sampler sampler = {};

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

	// HELPERS
	void setupVBO();
	void setupUBO(const std::string& image_path);
	void setupPipeline();
};


} // end namespace renderer