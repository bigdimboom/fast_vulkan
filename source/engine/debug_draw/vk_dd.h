#pragma once
#include "debug_draw.hpp"
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include "../vkapi/vk_ctx.h"
#include "../camera/free_camera.h"
#include "../renderer/irenderer.h"

#include <functional>

namespace dd
{

class VkDDRenderInterface final : public dd::RenderInterface, public renderer::IRenderer
{
public:
	VkDDRenderInterface(std::shared_ptr<vkapi::Context> vkCtx);
	~VkDDRenderInterface();

	void update(const glm::mat4& mvp);
	void prepare(FlushFlags flags = FlushFlags::FlushAll);
	void render() override;

	void drawLabel(glm::vec3 pos, glm::vec3 color, const char* text, float scale = 1.0f, camera::FreeCamera * cam = nullptr);
	void drawDemoExample(camera::FreeCamera& cam);


private:

	bool init();
	void shutdown();

	void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override;
	void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override;
	void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override;
	GlyphTextureHandle createGlyphTexture(int width, int height, const void* pixels) override;
	void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override;

	static inline std::int64_t getTimeMilliseconds()
	{
		const double seconds = SDL_GetTicks();
		return static_cast<std::int64_t>(seconds);
	}


private:
	std::shared_ptr<vkapi::Context> d_vkCtx;

	struct
	{
		std::shared_ptr<vkapi::BufferObject> pointSBO = nullptr; // staging buffer
		std::shared_ptr<vkapi::BufferObject> lineSBO = nullptr; // staging buffer
		std::shared_ptr<vkapi::BufferObject> pointVBO = nullptr; // vertex buffer
		std::shared_ptr<vkapi::BufferObject> lineVBO = nullptr; // vertex buffer
		vk::PipelineVertexInputStateCreateInfo inputState = {};
		vk::VertexInputBindingDescription inputBinding = {};
		std::vector<vk::VertexInputAttributeDescription> inputAttributes = {};
	}d_vertices;

	struct
	{
		std::shared_ptr<vkapi::BufferObject> textSBO = nullptr; // staging buffer
		std::shared_ptr<vkapi::BufferObject> textVBO = nullptr; // vertex buffer
		vk::PipelineVertexInputStateCreateInfo inputState = {};
		vk::VertexInputBindingDescription inputBinding = {};
		std::vector<vk::VertexInputAttributeDescription> inputAttributes = {};
	}d_text_vertices;

	struct
	{
		std::shared_ptr<vkapi::BufferObject> ubo = nullptr;
		vk::DescriptorSetLayoutBinding layoutBinding = {};
		vk::DescriptorSetLayout descriptorSetLayout = {};
		vk::DescriptorSet descriptorSet = {};
		vk::PipelineLayout pipelineLayout = {};
		vk::DescriptorBufferInfo descriptorBufferInfo = {};
		vk::WriteDescriptorSet writeDescriptorSet = {};
		glm::mat4 mvp_matrix = glm::mat4(1.0f);
	}d_mvp;


	struct
	{
		std::shared_ptr<vkapi::ImageObject> image;
		vk::ImageView imageView;
		vk::Sampler sampler = {};

		glm::vec2 screenDimensions = {};
		std::shared_ptr<vkapi::BufferObject> ubo = nullptr;

		std::vector<vk::DescriptorSetLayoutBinding> bindings = {};

		vk::DescriptorSetLayout descriptorSetLayout = {};

		vk::DescriptorSet descriptorSet = {};

		vk::PipelineLayout pipelineLayout = {};

		vk::DescriptorBufferInfo descriptorBufferInfo = {};
		vk::DescriptorImageInfo descriptorImageInfo = {};

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

	}d_text;


	vk::Viewport d_viewport = {};
	vk::Rect2D d_renderArea = {};

	struct
	{
		vk::ShaderModule vert = {};
		vk::ShaderModule frag = {};
		vk::PipelineCache pipelineCache = {};
		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {};
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStatePoint = {};
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateLine = {};
		vk::PipelineViewportStateCreateInfo viewportState = {};
		vk::PipelineRasterizationStateCreateInfo rasterState = {};
		vk::PipelineMultisampleStateCreateInfo multisampleState = {};
		vk::PipelineDepthStencilStateCreateInfo depthStencilStateDepthTestEnabled = {};
		vk::PipelineDepthStencilStateCreateInfo depthStencilStateDepthTestDisabled = {};
		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {};
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		std::vector<vk::DynamicState> dynamicStateList;
		vk::PipelineDynamicStateCreateInfo dynamicState;

		vk::Pipeline pipelineHandle_depthTestEnabled_point;
		vk::Pipeline pipelineHandle_depthTestDisabled_point;
		vk::Pipeline pipelineHandle_depthTestEnabled_line;
		vk::Pipeline pipelineHandle_depthTestDisabled_line;

	}d_linePointPipeline;

	struct
	{
		vk::ShaderModule vert = {};
		vk::ShaderModule frag = {};
		vk::PipelineCache pipelineCache = {};
		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {};
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		vk::PipelineViewportStateCreateInfo viewportState = {};
		vk::PipelineRasterizationStateCreateInfo rasterState = {};
		vk::PipelineMultisampleStateCreateInfo multisampleState = {};
		vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {};
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		std::vector<vk::DynamicState> dynamicStateList;

		vk::PipelineDynamicStateCreateInfo dynamicState;

		vk::Pipeline pipeline;
	}d_textPipeline;

	// defered draw command recordings.
	std::vector<std::function<void(void)>> d_draws;

	// HELPERS
	void setupVertexBuffers();
	void setupUniformBuffers();
	void setupTextUniforms();
	void setupPipelines();

};

} // end namespace dd