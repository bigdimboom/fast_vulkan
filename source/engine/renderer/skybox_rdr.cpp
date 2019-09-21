#include "skybox_rdr.h"

#define STB_IMAGE_IMPLEMENTATION
#include  "../util/stb_image.h"
#include <assert.h>
#include <SDL2/SDL.h>

#include "../app/system_mgr.h"

#define DEFAULT_TEXTURE "assets/default.png"

namespace renderer
{


SkyboxRdr::SkyboxRdr(std::shared_ptr<vkapi::Context> vkCtx, std::shared_ptr<camera::FreeCamera> cam, const std::vector<std::string>& faces_image_files)
	: d_vkCtx(vkCtx)
	, d_cam(cam)
{
	assert(cam);
	d_mvp.proj = cam->proj();
	d_mvp.view = cam->view();

	if (!loadCubeMap(faces_image_files))
	{
		SDL_Log("unable to load sky box textures; loading debug texture..");
		std::vector<std::string> debug;
		debug.resize(6, DEFAULT_TEXTURE);
		if (!loadCubeMap(debug))
		{
			SDL_Log("unable to load default textures");
			throw "error";
		}
	}

	setupVBO();
	setupUBO();
	setupPipeline();
	setViewport(d_vkCtx->viewport());
	setRenderArea(d_vkCtx->renderArea());
}

SkyboxRdr::~SkyboxRdr()
{
	// TODO:

	d_vkCtx->vkDevice().destroySampler(d_ubo.cubemapSampler);
	d_vkCtx->vkDevice().destroyImageView(d_ubo.cubemapView);
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

void SkyboxRdr::render()
{
	if (d_cam)
	{
		d_mvp.view = d_cam->view();
		d_mvp.proj = d_cam->proj();
	}

	d_vkCtx->upload(*d_ubo.mvp_buf, &d_mvp, sizeof(MVP));

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

void SkyboxRdr::setViewport(vk::Viewport viewport)
{
	d_viewport = viewport;
}

void SkyboxRdr::setRenderArea(vk::Rect2D scissor)
{
	d_renderArea = scissor;
}

// HELPERS
bool SkyboxRdr::loadCubeMap(const std::vector<std::string>& faces)
{
	if (faces.size() != 6)
	{
		return false;
	}

	int channels = 4;
	int width = 0, height = 0, nrComponents = 0;
	if (!stbi_load(faces[0].c_str(), &width, &height, &nrComponents, STBI_rgb_alpha))
	{
		return false;
	}

	vk::Format format = vk::Format::eR8G8B8A8Unorm;

	VmaAllocationCreateInfo image_alloc_info = {};
	image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	auto familyQueueIndex = d_vkCtx->familyQueueIndex();
	auto texture = d_vkCtx->createSharedImageObject(vk::ImageCreateInfo(
		vk::ImageCreateFlagBits::eCubeCompatible,
		vk::ImageType::e2D,
		format,
		vk::Extent3D(vk::Extent2D(width, height), 1),
		1u, 6u,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive,
		1, &familyQueueIndex,
		vk::ImageLayout::eUndefined),
		image_alloc_info);

	auto stagebuffer = d_vkCtx->createStagingBufferObject(width * height * channels * 6);
	std::size_t offset = 0;
	std::vector<vk::BufferImageCopy> bufferCopyRegions;

	for (unsigned int face_id = 0; face_id < faces.size(); face_id++)
	{
		unsigned char* data = stbi_load(faces[face_id].c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);
		if (data)
		{
			d_vkCtx->upload(*stagebuffer, data, width * height * channels, offset);


			vk::BufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face_id;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			stbi_image_free(data);
		}
		else
		{
			SDL_Log("Cubemap texture failed to load at path: %s\n", faces[face_id].c_str());
			return false;
		}

		offset += width * height * channels;
	}

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	vk::ImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;

	d_vkCtx->transitionImageLayout(texture->image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, subresourceRange);
	d_vkCtx->copy(texture->image, stagebuffer->buffer, bufferCopyRegions, vk::ImageLayout::eTransferDstOptimal);
	d_vkCtx->transitionImageLayout(texture->image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, subresourceRange);


	d_ubo.cubemap = texture;
	d_ubo.imageFormat = format;

	return true;
}

void SkyboxRdr::setupVBO()
{
	std::vector<float> skyboxVertices = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	d_bufferData.nVerts = (int)skyboxVertices.size() / 3;
	d_bufferData.vbo = d_vkCtx->createVertexBufferObject(skyboxVertices);

	d_bufferData.inputBinding.binding = 0;
	d_bufferData.inputBinding.inputRate = vk::VertexInputRate::eVertex;
	d_bufferData.inputBinding.stride = sizeof(float) * 3;

	d_bufferData.inputAttributes.resize(1);
	d_bufferData.inputAttributes[0].binding = 0;
	d_bufferData.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
	d_bufferData.inputAttributes[0].location = 0;
	d_bufferData.inputAttributes[0].offset = 0;

	d_bufferData.inputState = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		1,
		&d_bufferData.inputBinding,
		static_cast<uint32_t>(d_bufferData.inputAttributes.size()),
		d_bufferData.inputAttributes.data()
	);
}

void SkyboxRdr::setupUBO()
{
	d_ubo.mvp_buf = d_vkCtx->createUniformBufferObject(sizeof(MVP));

	d_ubo.cubemapView = d_vkCtx->vkDevice().createImageView(vk::ImageViewCreateInfo(
		vk::ImageViewCreateFlags(),
		d_ubo.cubemap->image,
		vk::ImageViewType::eCube,
		d_ubo.imageFormat,
		vk::ComponentMapping(),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6)
	));

	d_ubo.cubemapSampler =
		d_vkCtx->vkDevice().createSampler(vk::SamplerCreateInfo(
		vk::SamplerCreateFlags(),
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		0.0f, VK_FALSE, 1.0f,
		VK_FALSE, vk::CompareOp::eNever,
		0.0f, 0.0f,
		vk::BorderColor::eFloatOpaqueWhite
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

	d_ubo.descriptorBufferInfo.buffer = d_ubo.mvp_buf->buffer;
	d_ubo.descriptorBufferInfo.offset = 0;
	d_ubo.descriptorBufferInfo.range = sizeof(MVP);

	d_ubo.descriptorImageInfo.sampler = d_ubo.cubemapSampler;
	d_ubo.descriptorImageInfo.imageView = d_ubo.cubemapView;
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

void SkyboxRdr::setupPipeline()
{
	//shader
	d_pipeline.vs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "skybox.vert.spv"
	);

	d_pipeline.fs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "skybox.frag.spv"
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