#include "textured_cube_rdr.h"
#include <assert.h>
#include "../util/image_utils.h"
#include "../app/system_mgr.h"

#define DEFAULT_TEXTURE "assets/default.png"

namespace renderer
{


TexturedCubeRdr::TexturedCubeRdr(std::shared_ptr<vkapi::Context> vkCtx, std::shared_ptr<camera::FreeCamera> cam, const char* file)
	: d_vkCtx(vkCtx)
	, d_cam(cam)
{
	assert(d_cam && d_vkCtx);
	d_mvp.model = glm::mat4(1.0);
	d_mvp.view = d_cam->view();
	d_mvp.proj = d_cam->proj();

	setViewport(d_vkCtx->viewport());
	setRenderAera(d_vkCtx->renderArea());

	std::string texture_file(DEFAULT_TEXTURE);

	if (file)
	{
		texture_file = std::string(file);
	}

	setupVBO();
	setupUBO(texture_file);
	setupPipeline();
}

TexturedCubeRdr::~TexturedCubeRdr()
{
	d_vkCtx->vkDevice().destroySampler(d_ubo.sampler);
	d_vkCtx->vkDevice().destroyImageView(d_ubo.textureView);
	d_vkCtx->vkDevice().destroyDescriptorSetLayout(d_ubo.descriptorSetLayout);
	d_vkCtx->vkDevice().freeDescriptorSets(d_vkCtx->vkDescriptorPool(), d_ubo.descriptorSet);
	d_vkCtx->vkDevice().destroyPipelineLayout(d_ubo.pipelineLayout);

	d_vkCtx->vkDevice().destroyPipeline(d_pipeline.pipeline);
	if (d_pipeline.pipelineCache)
	{
		d_vkCtx->vkDevice().destroyPipelineCache(d_pipeline.pipelineCache);
	}
	d_vkCtx->vkDevice().destroyShaderModule(d_pipeline.vs);
	d_vkCtx->vkDevice().destroyShaderModule(d_pipeline.fs);
}

void TexturedCubeRdr::setTransform(const glm::mat4& xfm)
{
	d_mvp.model = xfm;
}

void TexturedCubeRdr::setCamera(std::shared_ptr<camera::FreeCamera> cam)
{
	d_cam = cam;
}

void TexturedCubeRdr::setViewport(vk::Viewport port)
{
	d_viewport = port;
}

void TexturedCubeRdr::setRenderAera(vk::Rect2D area)
{
	d_renderArea = area;
}

void TexturedCubeRdr::render()
{
	if (d_cam)
	{
		d_mvp.view = d_cam->view();
		d_mvp.proj = d_cam->proj();
	}

	d_vkCtx->upload(*d_ubo.mvpBuffer, &d_mvp, sizeof(MVP));

	auto cmd = d_vkCtx->commandBuffer();

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, d_pipeline.pipeline);

	cmd.setViewport(0, 1, &d_viewport);
	cmd.setScissor(0, 1, &d_renderArea);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		d_ubo.pipelineLayout,
		0, d_ubo.descriptorSet,
		nullptr
	);

	vk::DeviceSize offsets = 0;
	cmd.bindVertexBuffers(0, d_bufferData.vbo->buffer, offsets);
	cmd.draw(d_bufferData.nVerts, 1, 0, 0);
}

// HELPERS
void TexturedCubeRdr::setupVBO()
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	float cubeVertices[] = {
		// positions          // normals           // uv
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};

	auto size = sizeof(cubeVertices) / sizeof(Vertex);
	std::vector<Vertex> cube_verts;
	cube_verts.resize(size);
	memcpy(cube_verts.data(), cubeVertices, sizeof(cubeVertices));

	d_bufferData.nVerts = static_cast<int>(cube_verts.size());
	d_bufferData.vbo = d_vkCtx->createVertexBufferObject(cube_verts);

	d_bufferData.inputBinding.binding = 0;
	d_bufferData.inputBinding.inputRate = vk::VertexInputRate::eVertex;
	d_bufferData.inputBinding.stride = sizeof(Vertex);

	std::uint32_t offset = 0;
	d_bufferData.inputAttributes.resize(3);

	d_bufferData.inputAttributes[0].binding = 0;
	d_bufferData.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
	d_bufferData.inputAttributes[0].location = 0;
	d_bufferData.inputAttributes[0].offset = offset;
	offset += sizeof(glm::vec3);

	d_bufferData.inputAttributes[1].binding = 0;
	d_bufferData.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
	d_bufferData.inputAttributes[1].location = 1;
	d_bufferData.inputAttributes[1].offset = offset;
	offset += sizeof(glm::vec3);

	d_bufferData.inputAttributes[2].binding = 0;
	d_bufferData.inputAttributes[2].format = vk::Format::eR32G32Sfloat;
	d_bufferData.inputAttributes[2].location = 2;
	d_bufferData.inputAttributes[2].offset = offset;

	d_bufferData.inputState = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		1, &d_bufferData.inputBinding,
		static_cast<uint32_t>(d_bufferData.inputAttributes.size()),
		d_bufferData.inputAttributes.data()
	);
}

void TexturedCubeRdr::setupUBO(const std::string& image_path)
{
	d_ubo.mvpBuffer = d_vkCtx->createUniformBufferObject(sizeof(MVP));

	std::vector<uint8_t> image_data; int width = 0, height = 0;
	if (!util::ImageUtility::loadPNG(image_path, image_data, width, height))
	{
		throw("can't load image data");
	}

	d_ubo.texture2D = d_vkCtx->createTextureImage2D(width, height, vk::Format::eR8G8B8A8Unorm, image_data.data());
	d_ubo.textureView = d_vkCtx->vkDevice().createImageView(vk::ImageViewCreateInfo(
		vk::ImageViewCreateFlags(),
		d_ubo.texture2D->image,
		vk::ImageViewType::e2D,
		vk::Format::eR8G8B8A8Unorm,
		vk::ComponentMapping(),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	));

	d_ubo.sampler =
		d_vkCtx->vkDevice().createSampler(vk::SamplerCreateInfo(
		vk::SamplerCreateFlags(),
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge
		));

	d_ubo.layoutBindings = {
		vk::DescriptorSetLayoutBinding(
			0, vk::DescriptorType::eUniformBuffer,
			1, vk::ShaderStageFlagBits::eVertex
		),
		vk::DescriptorSetLayoutBinding(
			1, vk::DescriptorType::eCombinedImageSampler,
			1,  vk::ShaderStageFlagBits::eFragment
	)
	};

	d_ubo.descriptorSetLayout =
		d_vkCtx->vkDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		static_cast<uint32_t>(d_ubo.layoutBindings.size()),
		d_ubo.layoutBindings.data()
		));

	d_ubo.descriptorSet =
		d_vkCtx->vkDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo(
		d_vkCtx->vkDescriptorPool(),
		1, &d_ubo.descriptorSetLayout
		))[0];

	d_ubo.pipelineLayout =
		d_vkCtx->vkDevice().createPipelineLayout(vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		1, &d_ubo.descriptorSetLayout
		));

	d_ubo.descriptorBufferInfo.buffer = d_ubo.mvpBuffer->buffer;
	d_ubo.descriptorBufferInfo.offset = 0;
	d_ubo.descriptorBufferInfo.range = sizeof(glm::vec2);

	d_ubo.descriptorImageInfo.sampler = d_ubo.sampler;
	d_ubo.descriptorImageInfo.imageView = d_ubo.textureView;
	d_ubo.descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	// TODO:
	d_ubo.writeDescriptorSets = {
		vk::WriteDescriptorSet(d_ubo.descriptorSet, 0, 0, 1,
			vk::DescriptorType::eUniformBuffer, nullptr, &d_ubo.descriptorBufferInfo
		),
		vk::WriteDescriptorSet(d_ubo.descriptorSet, 1, 0, 1,
			vk::DescriptorType::eCombinedImageSampler, &d_ubo.descriptorImageInfo, nullptr
		)
	};

	d_vkCtx->vkDevice().updateDescriptorSets(d_ubo.writeDescriptorSets, nullptr);
}

void TexturedCubeRdr::setupPipeline()
{
	//shader
	d_pipeline.vs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "cube.vert.spv"
	);

	d_pipeline.fs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "cube.frag.spv"
	);

	d_pipeline.shaderCreateInfos = {
		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			d_pipeline.vs, "main"
		),
		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			d_pipeline.fs, "main"
		)
	};

	// setup fix functions
	auto assemblyState = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList
	);

	auto viewportState = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &d_viewport,
		1, &d_renderArea
	);

	auto rasterState = vk::PipelineRasterizationStateCreateInfo(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE,
		0,
		0,
		0,
		1.0f
	);

	auto multisampleState = vk::PipelineMultisampleStateCreateInfo(
		vk::PipelineMultisampleStateCreateFlags(),
		vk::SampleCountFlagBits::e1
	);

	auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo(
		vk::PipelineDepthStencilStateCreateFlags(),
		VK_TRUE,
		VK_TRUE,
		vk::CompareOp::eLessOrEqual,
		VK_FALSE,
		VK_FALSE,
		vk::StencilOpState(),
		vk::StencilOpState(),
		0,
		0
	);

	auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState(
		VK_FALSE,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eOne,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA)
	);

	//auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState(
	//	VK_TRUE,
	//	vk::BlendFactor::eSrcAlpha,
	//	vk::BlendFactor::eOneMinusSrcAlpha,
	//	vk::BlendOp::eAdd,
	//	vk::BlendFactor::eSrcAlpha,
	//	vk::BlendFactor::eOneMinusSrcAlpha,
	//	vk::BlendOp::eAdd,
	//	vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR |
	//	vk::ColorComponentFlagBits::eG |
	//	vk::ColorComponentFlagBits::eB |
	//	vk::ColorComponentFlagBits::eA)
	//);

	auto colorBlendState = vk::PipelineColorBlendStateCreateInfo(
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eClear,
		1, &colorBlendAttachment
	);

	auto dynamicStateList = std::vector<vk::DynamicState>{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	auto dynamicState = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		static_cast<uint32_t>(dynamicStateList.size()),
		dynamicStateList.data()
	);

	d_pipeline.pipeline = d_vkCtx->vkDevice().createGraphicsPipeline(
		d_pipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_pipeline.shaderCreateInfos.size()),
		d_pipeline.shaderCreateInfos.data(),
		&d_bufferData.inputState,
		&assemblyState,
		nullptr,
		&viewportState,
		&rasterState,
		&multisampleState,
		&depthStencilState,
		&colorBlendState,
		&dynamicState,
		d_ubo.pipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
	)
	);

}

} // end namespace renderer