#include "static_model_renderer.h"
#include <assert.h>
#include "../mesh/util.h"
#include "../app/system_mgr.h"
#include "../debug_draw/debug_draw.hpp"

namespace renderer
{



StaticModelRenderer::StaticModelRenderer(std::shared_ptr<vkapi::Context> vkCtx, std::shared_ptr<camera::FreeCamera> camera)
	: d_vkCtx(vkCtx)
	, d_camera(camera)
{
	assert(d_camera);
	assert(d_vkCtx);
	d_viewport = vkCtx->viewport();
	d_renderArea = vkCtx->renderArea();

	d_mvp.model = glm::mat4(1.0f);
	d_mvp.view = d_camera->view();
	d_mvp.proj = d_camera->proj();
}

StaticModelRenderer::~StaticModelRenderer()
{
	if (d_vertexInput.vbo)
	{
		//d_vkCtx->vkDevice().destroySampler(d_ubo.cubemapSampler);
		//d_vkCtx->vkDevice().destroyImageView(d_ubo.cubemapView);

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
}

void StaticModelRenderer::setCamera(std::shared_ptr<camera::FreeCamera> cam)
{
	assert(cam);
	d_camera = cam;
}

void StaticModelRenderer::setViewport(int x, int y, int width, int height)
{
	d_viewport.x = x;
	d_viewport.y = y;
	d_viewport.width = width;
	d_viewport.height = height;
}

void StaticModelRenderer::setModel(std::shared_ptr<mesh::StaticModel> model, const glm::mat4& transform)
{
	d_input.smodel = model;
	d_input.transform = transform;
	//d_mvp.model = transform;
}

bool StaticModelRenderer::build(bool clear_host_data)
{
	if (!d_input.smodel)
	{
		SDL_Log("set input first");
		return false;
	}

	if (!buildVBO())
	{
		return false;
	}

	buildIBO();
	buildUBO();
	buildPipeline();

	if (clear_host_data)
	{
		d_input.smodel = nullptr;
	}
}

void StaticModelRenderer::render()
{
	if (!d_vertexInput.vbo)
	{
		SDL_Log("did not have vbo built, skip rendering.");
		return;
	}

	d_mvp.view = d_camera->view();
	d_mvp.proj = d_camera->proj();

	d_vkCtx->upload(*d_ubo.mvp_buffer, &d_mvp, sizeof(MVP));

	auto cmd = d_vkCtx->commandBuffer();

	cmd.setViewport(0, 1, &d_viewport);
	cmd.setScissor(0, 1, &d_renderArea);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, d_pipeline.pipeline);
	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		d_ubo.pipelineLayout,
		0, d_ubo.descriptorSet,
		nullptr
	);

	for (int i = 0; i < d_vertexInput.offsets.size(); ++i)
	{
		cmd.bindVertexBuffers(0, d_vertexInput.vbo->buffer, d_vertexInput.offsets[i]);
		cmd.bindIndexBuffer(d_indexInput.ibos[i]->buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(d_indexInput.index_count[i], 1, 0, 0, 0);
	}

	//d_tree->traverse([this](const glm::vec3& min, const glm::vec3& max, std::vector<octree::DrawMeshData>* data) {

	//	static glm::vec3 color(1.0, 0.0, 0.0);
	//	dd::aabb(*((ddVec3_In*)& min), *((ddVec3_In*)& max), *((ddVec3_In*)& color));

	//	if (data)
	//	{
	//		auto& stuff = *data;
	//		for (auto& elem : stuff)
	//		{
	//			//cmd.bindVertexBuffers(0, elem.vbo->buffer, elem.vbo_offset);
	//			//cmd.bindIndexBuffer(elem.ibo->buffer, vk::DeviceSize(0), vk::IndexType::eUint32);
	//			//cmd.drawIndexed(elem.draw_index_count, 1, 0,/* elem.vbo_offset*/0, 0);
	//		}
	//	}
	//	return true;
	//});
}

// HELPERS
bool StaticModelRenderer::buildVBO()
{
	std::vector<mesh::Vertex> result;
	std::vector<mesh::Vertex> tmp;
	std::size_t offset = 0;

	for (auto& mesh : d_input.smodel->meshes())
	{
		mesh::Utility::transformPointCloud(*mesh, d_input.transform);
		mesh::Utility::computeNormals(*mesh, true);
		mesh::Utility::createVertexArray(*mesh, tmp);

		d_vertexInput.offsets.push_back(offset);
		std::copy(tmp.begin(), tmp.end(), std::back_inserter(result));
		offset += tmp.size() * sizeof(mesh::Vertex);
	}

	offset = 0;
	d_vertexInput.inputBinding.binding = 0;
	d_vertexInput.inputBinding.stride = sizeof(mesh::Vertex);
	d_vertexInput.inputBinding.inputRate = vk::VertexInputRate::eVertex;

	d_vertexInput.inputAttributes.resize(3);
	d_vertexInput.inputAttributes[0].binding = 0;
	d_vertexInput.inputAttributes[0].location = 0;
	d_vertexInput.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
	d_vertexInput.inputAttributes[0].offset = offset;
	offset += sizeof(mesh::Position);

	d_vertexInput.inputAttributes[1].binding = 0;
	d_vertexInput.inputAttributes[1].location = 1;
	d_vertexInput.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
	d_vertexInput.inputAttributes[1].offset = offset;
	offset += sizeof(mesh::Normal);

	d_vertexInput.inputAttributes[2].binding = 0;
	d_vertexInput.inputAttributes[2].location = 2;
	d_vertexInput.inputAttributes[2].format = vk::Format::eR32G32B32A32Sfloat;
	d_vertexInput.inputAttributes[2].offset = offset;
	offset += sizeof(mesh::Color);

	//d_vertexInput.inputAttributes[3].binding = 0;
	//d_vertexInput.inputAttributes[3].location = 3;
	//d_vertexInput.inputAttributes[3].format = vk::Format::eR32G32Sfloat;
	//d_vertexInput.inputAttributes[3].offset = offset;
	//offset += sizeof(mesh::UV);

	// TODO:
	d_vertexInput.vbo = d_vkCtx->createVertexBufferObject(result);

	d_vertexInput.inputState = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		1,
		&d_vertexInput.inputBinding,
		static_cast<uint32_t>(d_vertexInput.inputAttributes.size()),
		d_vertexInput.inputAttributes.data()
	);


	//d_tree = std::make_unique<octree::DrawMeshOctree>(glm::vec3{ -1000.0f, -1000.0f, -1000.0f }, 2000.0f, 10);

	//for (size_t i = 0; i < d_input.smodel->meshes().size(); ++i)
	//{
	//	auto& mesh = d_input.smodel->meshes()[i];
	//	auto& vertices = mesh->positions;
	//	auto& indices = mesh->indices;

	//	for (size_t ii = 0; ii < indices.size(); ii += 3)
	//	{
	//		auto pt = vertices[indices[ii]];

	//		octree::DrawMeshOctree::ErrorCode err;
	//		auto& data = d_tree->push(pt, err);

	//		data.resize(d_input.smodel->meshes().size());
	//		data[i].vbo = d_vertexInput.vbo;
	//		data[i].vbo_offset = d_vertexInput.offsets[i];

	//		if (err == 0)
	//		{
	//			data[i].ibo_host.push_back(indices[ii]);
	//			data[i].ibo_host.push_back(indices[ii + 1]);
	//			data[i].ibo_host.push_back(indices[ii + 2]);
	//		}
	//	}
	//}

	//d_tree->linearProcess([this](octree::DrawMeshNode& node) {
	//	assert(node.childrenFlags == 0);
	//	//for (auto& elem : *node.data)
	//	//{
	//	//	elem.ibo = d_vkCtx->createIndexBufferObject(elem.ibo_host);
	//	//	elem.draw_index_count = elem.ibo_host.size();
	//	//	elem.ibo_host.clear();
	//	//}
	//}, true);

	return true;
}

void StaticModelRenderer::buildIBO()
{
	auto& meshes = d_input.smodel->meshes();
	for (int i = 0; i < meshes.size(); ++i)
	{
		d_indexInput.index_count.push_back(meshes[i]->indices.size());
		d_indexInput.ibos.push_back(d_vkCtx->createIndexBufferObject(meshes[i]->indices));
	}
}

void StaticModelRenderer::buildUBO()
{
	d_ubo.mvp_buffer = d_vkCtx->createUniformBufferObject(sizeof(MVP));

	d_ubo.layoutBindings = {
		vk::DescriptorSetLayoutBinding(
			0, vk::DescriptorType::eUniformBuffer,
			1, vk::ShaderStageFlagBits::eVertex
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

	//vk::PushConstantRange pushConstantRange(
	//	vk::ShaderStageFlagBits::eFragment,
	//	0, sizeof(float)
	//);

	d_ubo.pipelineLayout =
		d_vkCtx->vkDevice().createPipelineLayout(vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		1, &d_ubo.descriptorSetLayout,
		0, nullptr
		));

	d_ubo.mvp_buffer_info.buffer = d_ubo.mvp_buffer->buffer;
	d_ubo.mvp_buffer_info.range = sizeof(MVP);
	d_ubo.mvp_buffer_info.offset = 0;

	d_ubo.writeDescriptorSets = {
	vk::WriteDescriptorSet(d_ubo.descriptorSet, 0, 0, 1,
		vk::DescriptorType::eUniformBuffer, nullptr, &d_ubo.mvp_buffer_info
		)
	};

	d_vkCtx->vkDevice().updateDescriptorSets(d_ubo.writeDescriptorSets, nullptr);
}

void StaticModelRenderer::buildPipeline()
{
	//shader
	//TODO:
	d_pipeline.vs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "model.vert.spv"
	);

	d_pipeline.fs = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "model.frag.spv"
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
		&d_vertexInput.inputState,
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