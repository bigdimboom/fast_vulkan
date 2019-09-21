#include "vk_dd.h"
#define DEBUG_DRAW_IMPLEMENTATION
#include "debug_draw.hpp"

//#include <stddef.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../app/system_mgr.h"
#include <assert.h>



namespace dd
{



VkDDRenderInterface::VkDDRenderInterface(std::shared_ptr<vkapi::Context> vkCtx)
	: d_vkCtx(vkCtx)
{
	assert(d_vkCtx);

	setupVertexBuffers();
	setupUniformBuffers();
	setupPipelines();

	init();

	std::printf("DDVulkanRenderInterface ready!\n\n");
}

VkDDRenderInterface::~VkDDRenderInterface()
{
	shutdown();
	/////////////////
	d_vkCtx->vkDevice().destroyPipelineLayout(d_mvp.pipelineLayout);
	d_vkCtx->vkDevice().destroyDescriptorSetLayout(d_mvp.descriptorSetLayout);
	d_vkCtx->vkDevice().freeDescriptorSets(d_vkCtx->vkDescriptorPool(), d_mvp.descriptorSet);

	if (d_linePointPipeline.pipelineHandle_depthTestEnabled_point)
		d_vkCtx->vkDevice().destroyPipeline(d_linePointPipeline.pipelineHandle_depthTestEnabled_point);
	if (d_linePointPipeline.pipelineHandle_depthTestDisabled_point)
		d_vkCtx->vkDevice().destroyPipeline(d_linePointPipeline.pipelineHandle_depthTestDisabled_point);
	if (d_linePointPipeline.pipelineHandle_depthTestEnabled_line)
		d_vkCtx->vkDevice().destroyPipeline(d_linePointPipeline.pipelineHandle_depthTestEnabled_line);
	if (d_linePointPipeline.pipelineHandle_depthTestDisabled_line)
		d_vkCtx->vkDevice().destroyPipeline(d_linePointPipeline.pipelineHandle_depthTestDisabled_line);

	d_vkCtx->vkDevice().destroyPipelineCache(d_linePointPipeline.pipelineCache);
	d_vkCtx->vkDevice().destroyPipelineCache(d_textPipeline.pipelineCache);

	//d_vkCtx->vkDevice().destroyImageView(d_text.imageView);
	d_vkCtx->vkDevice().destroySampler(d_text.sampler);

	d_vkCtx->vkDevice().destroyPipeline(d_textPipeline.pipeline);

	d_text.image = nullptr;

	d_vkCtx->vkDevice().destroyPipelineLayout(d_text.pipelineLayout);
	d_vkCtx->vkDevice().destroyDescriptorSetLayout(d_text.descriptorSetLayout);

	d_vkCtx->vkDevice().destroyShaderModule(d_textPipeline.vert);
	d_vkCtx->vkDevice().destroyShaderModule(d_textPipeline.frag);
	d_vkCtx->vkDevice().destroyShaderModule(d_linePointPipeline.vert);
	d_vkCtx->vkDevice().destroyShaderModule(d_linePointPipeline.frag);

	d_draws.clear();
}

void VkDDRenderInterface::update(const glm::mat4& mvp)
{
	d_mvp.mvp_matrix = mvp;
	d_vkCtx->upload(*d_mvp.ubo, glm::value_ptr(d_mvp.mvp_matrix), sizeof(glm::mat4));
}

void VkDDRenderInterface::prepare(FlushFlags flags)
{
	d_draws.clear();
	dd::flush(getTimeMilliseconds(), flags);
}

void VkDDRenderInterface::render()
{
	for (auto& elem : d_draws)
	{
		elem();
	}
}

void VkDDRenderInterface::drawLabel(glm::vec3 pos, glm::vec3 color, const char* text, float scale, camera::FreeCamera* camera)
{
	if (camera && camera->isPointInsideFrustum(pos))
	{
		const ddVec3 textColor = { color.r, color.g, color.b };
		const ddVec3 textPos = { pos.x, pos.y, pos.z };
		dd::projectedText(text, textPos, textColor, glm::value_ptr(camera->viewProj()),
			(int)d_viewport.x, (int)d_viewport.y, (int)d_viewport.width, (int)d_viewport.height, scale);
	}

}

void VkDDRenderInterface::drawDemoExample(camera::FreeCamera& camera)
{
	auto drawGrid = [&, this]()
	{
		dd::xzSquareGrid(-50.0f, 50.0f, -1.0f, 1.7f, dd::colors::Green); // Grid from -50 to +50 in both X & Z
	};

	auto drawLabel = [&, this](ddVec3_In pos, const char* name)
	{
		// Only draw labels inside the camera frustum.
		if (camera.isPointInsideFrustum(glm::vec3(pos[0], pos[1], pos[2])))
		{
			const ddVec3 textColor = { 0.8f, 0.8f, 1.0f };
			dd::projectedText(name, pos, textColor, glm::value_ptr(camera.viewProj()),
				(int)d_viewport.x, (int)d_viewport.y, (int)d_viewport.width, (int)d_viewport.height, 0.5f);
		}
	};

	auto drawMiscObjects = [&]()
	{
		// Start a row of objects at this position:
		ddVec3 origin = { -15.0f, 0.0f, 0.0f };

		// Box with a point at it's center:
		drawLabel(origin, "box");
		dd::box(origin, dd::colors::Blue, 1.5f, 1.5f, 1.5f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 3.0f;

		// Sphere with a point at its center
		drawLabel(origin, "sphere");
		dd::sphere(origin, dd::colors::Red, 1.0f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 4.0f;

		// Two cones, one open and one closed:
		const ddVec3 condeDir = { 0.0f, 2.5f, 0.0f };
		origin[1] -= 1.0f;

		drawLabel(origin, "cone (open)");
		dd::cone(origin, condeDir, dd::colors::Yellow, 1.0f, 2.0f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 4.0f;

		drawLabel(origin, "cone (closed)");
		dd::cone(origin, condeDir, dd::colors::Cyan, 0.0f, 1.0f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 4.0f;

		// Axis-aligned bounding box:
		const ddVec3 bbMins = { -1.0f, -0.9f, -1.0f };
		const ddVec3 bbMaxs = { 1.0f,  2.2f,  1.0f };
		const ddVec3 bbCenter = {
			(bbMins[0] + bbMaxs[0]) * 0.5f,
			(bbMins[1] + bbMaxs[1]) * 0.5f,
			(bbMins[2] + bbMaxs[2]) * 0.5f
		};
		drawLabel(origin, "AABB");
		dd::aabb(bbMins, bbMaxs, dd::colors::Orange);
		dd::point(bbCenter, dd::colors::White, 15.0f);

		// Move along the Z for another row:
		origin[0] = -15.0f;
		origin[2] += 5.0f;

		// A big arrow pointing up:
		const ddVec3 arrowFrom = { origin[0], origin[1], origin[2] };
		const ddVec3 arrowTo = { origin[0], origin[1] + 5.0f, origin[2] };
		drawLabel(arrowFrom, "arrow");
		dd::arrow(arrowFrom, arrowTo, dd::colors::Magenta, 1.0f);
		dd::point(arrowFrom, dd::colors::White, 15.0f);
		dd::point(arrowTo, dd::colors::White, 15.0f);
		origin[0] += 4.0f;

		// Plane with normal vector:
		const ddVec3 planeNormal = { 0.0f, 1.0f, 0.0f };
		drawLabel(origin, "plane");
		dd::plane(origin, planeNormal, dd::colors::Yellow, dd::colors::Blue, 1.5f, 1.0f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 4.0f;

		// Circle on the Y plane:
		drawLabel(origin, "circle");
		dd::circle(origin, planeNormal, dd::colors::Orange, 1.5f, 15.0f);
		dd::point(origin, dd::colors::White, 15.0f);
		origin[0] += 3.2f;

		// Tangent basis vectors:
		const ddVec3 normal = { 0.0f, 1.0f, 0.0f };
		const ddVec3 tangent = { 1.0f, 0.0f, 0.0f };
		const ddVec3 bitangent = { 0.0f, 0.0f, 1.0f };
		origin[1] += 0.1f;
		drawLabel(origin, "tangent basis");
		dd::tangentBasis(origin, normal, tangent, bitangent, 2.5f);
		dd::point(origin, dd::colors::White, 15.0f);

		// And a set of intersecting axes:
		origin[0] += 4.0f;
		origin[1] += 1.0f;
		drawLabel(origin, "cross");
		dd::cross(origin, 2.0f);
		dd::point(origin, dd::colors::White, 15.0f);
	};

	auto drawFrustum = [&, this]() {

		const ddVec3 color = { 0.8f, 0.3f, 1.0f };
		const ddVec3 origin = { -8.0f, 0.5f, 14.0f };
		drawLabel(origin, "frustum + axes");

		// The frustum will depict a fake camera:
		const glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.5f, 4.0f);
		const glm::mat4 view = glm::lookAt(glm::vec3(-8.0f, 0.5f, 14.0f), glm::vec3(-8.0f, 0.5f, -14.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 clip = glm::inverse(proj * view);
		dd::frustum(glm::value_ptr(clip), color);

		// A white dot at the eye position:
		dd::point(origin, dd::colors::White, 15.0f);

		// A set of arrows at the camera's origin/eye:
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.5f, 14.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(60.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		dd::axisTriad(glm::value_ptr(transform), 0.3f, 2.0f);
	};

	auto drawText = [&, this]()
	{
		// HUD text:
		const ddVec3 textColor = { 1.0f,  0.5f,  0.9f };
		const ddVec3 textPos2D = { 15.0, 500.0f, 0.0f };
		dd::screenText("Welcome to the Vulkan Debug Draw demo.\n\n"
			"[SPACE]  to toggle labels on/off\n"
			"[RETURN] to toggle grid on/off",
			textPos2D, textColor, 0.55f);
	};

	drawGrid();
	drawMiscObjects();
	drawFrustum();
	drawText();

}

bool VkDDRenderInterface::init()
{
	if (!dd::isInitialized())
	{
		return dd::initialize(this);
	}
	return true;
}

void VkDDRenderInterface::shutdown()
{
	if (dd::isInitialized())
	{
		dd::shutdown();
	}

}

void VkDDRenderInterface::drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled)
{
	assert(points != nullptr);
	assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

	vk::Pipeline pipeline = {};

	if (depthEnabled)
	{
		pipeline = d_linePointPipeline.pipelineHandle_depthTestEnabled_point;
	}
	else
	{
		pipeline = d_linePointPipeline.pipelineHandle_depthTestDisabled_point;
	}

	auto data_size = sizeof(dd::DrawVertex) * count;

	d_vkCtx->upload(*d_vertices.pointSBO, (void*)points, data_size);

	auto cmd = d_vkCtx->commandBuffer();
	vk::BufferCopy region(0, 0, data_size);
	cmd.copyBuffer(d_vertices.pointSBO->buffer, d_vertices.pointVBO->buffer, region);

	d_draws.push_back([cmd, this, count, pipeline] {
		cmd.setViewport(0, 1, &d_viewport);
		cmd.setScissor(0, 1, &d_renderArea);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			d_mvp.pipelineLayout,
			0,
			d_mvp.descriptorSet,
			nullptr
		);

		vk::DeviceSize offsets = 0;
		cmd.bindVertexBuffers(0, 1, &d_vertices.pointVBO->buffer, &offsets);
		cmd.draw(count, 1, 0, 0);
	});
}

void VkDDRenderInterface::drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled)
{
	assert(lines != nullptr);
	assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

	vk::Pipeline pipeline = {};

	if (depthEnabled)
	{
		pipeline = d_linePointPipeline.pipelineHandle_depthTestEnabled_line;
	}
	else
	{
		pipeline = d_linePointPipeline.pipelineHandle_depthTestDisabled_line;
	}

	auto data_size = sizeof(dd::DrawVertex) * count;

	d_vkCtx->upload(*d_vertices.lineSBO, (void*)lines, data_size);

	auto cmd = d_vkCtx->commandBuffer();

	vk::BufferCopy region(0, 0, data_size);
	cmd.copyBuffer(d_vertices.lineSBO->buffer, d_vertices.lineVBO->buffer, region);

	d_draws.push_back([cmd, this, count, pipeline]() {

		cmd.setViewport(0, 1, &d_viewport);
		cmd.setScissor(0, 1, &d_renderArea);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			d_mvp.pipelineLayout,
			0,
			d_mvp.descriptorSet,
			nullptr
		);

		vk::DeviceSize offsets = 0;
		cmd.bindVertexBuffers(0, 1, &d_vertices.lineVBO->buffer, &offsets);
		cmd.draw(count, 1, 0, 0);

	});

}

void VkDDRenderInterface::drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex)
{
	//SDL_Log("count : %d", count);

	// TODO:
	auto data_size = sizeof(dd::DrawVertex) * count;
	d_vkCtx->upload(*d_text_vertices.textSBO, (void*)glyphs, data_size);
	auto cmd = d_vkCtx->commandBuffer();
	vk::BufferCopy region(0, 0, data_size);
	cmd.copyBuffer(d_text_vertices.textSBO->buffer, d_text_vertices.textVBO->buffer, region);

	auto pipeline = d_textPipeline.pipeline;

	d_draws.push_back([cmd, this, count, pipeline]() {

		cmd.setViewport(0, 1, &d_viewport);
		cmd.setScissor(0, 1, &d_renderArea);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			d_text.pipelineLayout,
			0,
			d_text.descriptorSet,
			nullptr
		);

		vk::DeviceSize offsets = 0;
		cmd.bindVertexBuffers(0, 1, &d_text_vertices.textVBO->buffer, &offsets);
		cmd.draw(count, 1, 0, 0);

	});
}

GlyphTextureHandle VkDDRenderInterface::createGlyphTexture(int width, int height, const void* pixels)
{
	d_text.image = d_vkCtx->createTextureImage2D(width, height, vk::Format::eR8Unorm, (void*)pixels);
	d_text.imageView = d_vkCtx->vkDevice().createImageView(vk::ImageViewCreateInfo(
		vk::ImageViewCreateFlags(),
		d_text.image->image,
		vk::ImageViewType::e2D,
		vk::Format::eR8Unorm,
		vk::ComponentMapping(),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	));

	setupTextUniforms();

	return reinterpret_cast<dd::GlyphTextureHandle>(1);
}

void VkDDRenderInterface::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex)
{
	d_vkCtx->vkDevice().destroyImageView(d_text.imageView);
	d_text.image = nullptr;
}

void VkDDRenderInterface::setupVertexBuffers()
{
	std::printf("> DDRenderInterfaceCoreGL::setupVertexBuffers()\n");

	//
	// Lines/points vertex buffer:
	//
	{
		vk::BufferCreateInfo sbo_create_info = {};
		VmaAllocationCreateInfo sbo_alloc_info = {};
		sbo_create_info.size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex);
		sbo_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
		sbo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		d_vertices.lineSBO = d_vkCtx->createSharedBufferObject(sbo_create_info, sbo_alloc_info);
		d_vertices.pointSBO = d_vkCtx->createSharedBufferObject(sbo_create_info, sbo_alloc_info);

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex);
		vboInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		d_vertices.pointVBO = d_vkCtx->createSharedBufferObject(vboInfo, vboAllocInfo);
		d_vertices.lineVBO = d_vkCtx->createSharedBufferObject(vboInfo, vboAllocInfo);

		// Vertex input binding
		d_vertices.inputBinding.binding = 0;
		d_vertices.inputBinding.stride = sizeof(dd::DrawVertex);
		d_vertices.inputBinding.inputRate = vk::VertexInputRate::eVertex;

		std::uint32_t offset = 0;

		// Inpute attribute binding describe shader attribute locations and memory layouts
		// These match the following shader layout (see assets/shaders/triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		d_vertices.inputAttributes.resize(2);
		// Attribute location 0:  in_Position (vec3)
		d_vertices.inputAttributes[0].binding = 0;
		d_vertices.inputAttributes[0].location = 0;
		d_vertices.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
		d_vertices.inputAttributes[0].offset = offset;
		offset += sizeof(float) * 3;
		// Attribute location 1: in_ColorPointSize (vec4)
		d_vertices.inputAttributes[1].binding = 0;
		d_vertices.inputAttributes[1].location = 1;
		d_vertices.inputAttributes[1].format = vk::Format::eR32G32B32A32Sfloat;
		d_vertices.inputAttributes[1].offset = offset;

		// Assign to the vertex input state used for pipeline creation
		d_vertices.inputState.flags = vk::PipelineVertexInputStateCreateFlags();
		d_vertices.inputState.vertexBindingDescriptionCount = 1;
		d_vertices.inputState.pVertexBindingDescriptions = &d_vertices.inputBinding;
		d_vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(d_vertices.inputAttributes.size());
		d_vertices.inputState.pVertexAttributeDescriptions = d_vertices.inputAttributes.data();

	}

	//
	// Text rendering vertex buffer:
	//
	{
		vk::BufferCreateInfo sbo_create_info = {};
		VmaAllocationCreateInfo sbo_alloc_info = {};
		sbo_create_info.size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex);
		sbo_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
		sbo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		d_text_vertices.textSBO = d_vkCtx->createSharedBufferObject(sbo_create_info, sbo_alloc_info);

		vk::BufferCreateInfo vboInfo = {};
		VmaAllocationCreateInfo vboAllocInfo = {};
		vboInfo.size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex);
		vboInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		d_text_vertices.textVBO = d_vkCtx->createSharedBufferObject(vboInfo, vboAllocInfo);

		// Vertex input binding
		d_text_vertices.inputBinding.binding = 0;
		d_text_vertices.inputBinding.stride = sizeof(dd::DrawVertex);
		d_text_vertices.inputBinding.inputRate = vk::VertexInputRate::eVertex;

		std::uint32_t offset = 0;

		d_text_vertices.inputAttributes.resize(3);
		// in_Position (vec2)
		d_text_vertices.inputAttributes[0].binding = 0;
		d_text_vertices.inputAttributes[0].location = 0;
		d_text_vertices.inputAttributes[0].format = vk::Format::eR32G32Sfloat;
		d_text_vertices.inputAttributes[0].offset = offset;
		offset += sizeof(float) * 2;

		// in_TexCoords (vec2)
		d_text_vertices.inputAttributes[1].binding = 0;
		d_text_vertices.inputAttributes[1].location = 1;
		d_text_vertices.inputAttributes[1].format = vk::Format::eR32G32Sfloat;
		d_text_vertices.inputAttributes[1].offset = offset;
		offset += sizeof(float) * 2;

		// in_Color (vec4)
		d_text_vertices.inputAttributes[2].binding = 0;
		d_text_vertices.inputAttributes[2].location = 2;
		d_text_vertices.inputAttributes[2].format = vk::Format::eR32G32B32A32Sfloat;
		d_text_vertices.inputAttributes[2].offset = offset;


		// Assign to the vertex input state used for pipeline creation
		d_text_vertices.inputState.flags = vk::PipelineVertexInputStateCreateFlags();
		d_text_vertices.inputState.vertexBindingDescriptionCount = 1;
		d_text_vertices.inputState.pVertexBindingDescriptions = &d_text_vertices.inputBinding;
		d_text_vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(d_text_vertices.inputAttributes.size());
		d_text_vertices.inputState.pVertexAttributeDescriptions = d_text_vertices.inputAttributes.data();
	}
}


void VkDDRenderInterface::setupUniformBuffers()
{
	d_mvp.mvp_matrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 1.0f, 5000.0f);

	d_mvp.layoutBinding.binding = 0;
	d_mvp.layoutBinding.descriptorCount = 1;
	d_mvp.layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	d_mvp.layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	d_mvp.descriptorSetLayout = d_vkCtx->vkDevice().createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		1,
		&d_mvp.layoutBinding
	));

	d_mvp.descriptorSet = d_vkCtx->vkDevice().allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo(
		d_vkCtx->vkDescriptorPool(),
		1,
		&d_mvp.descriptorSetLayout
	))[0];

	d_mvp.pipelineLayout = d_vkCtx->vkDevice().createPipelineLayout(
		vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		1,
		&d_mvp.descriptorSetLayout,
		0,
		nullptr
	));


	vk::BufferCreateInfo ubo_create_info = {};
	VmaAllocationCreateInfo ubo_alloc_info = {};
	ubo_create_info.size = sizeof(glm::mat4);
	ubo_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	ubo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	d_mvp.ubo = d_vkCtx->createSharedBufferObject(ubo_create_info, ubo_alloc_info);

	d_mvp.descriptorBufferInfo.buffer = d_mvp.ubo->buffer;
	d_mvp.descriptorBufferInfo.offset = 0;
	d_mvp.descriptorBufferInfo.range = sizeof(glm::mat4);

	d_mvp.writeDescriptorSet = vk::WriteDescriptorSet(
		d_mvp.descriptorSet, // TODO:
		0,
		0,
		1,
		vk::DescriptorType::eUniformBuffer,
		nullptr,
		&d_mvp.descriptorBufferInfo,
		nullptr
	);

	d_vkCtx->vkDevice().updateDescriptorSets(d_mvp.writeDescriptorSet, nullptr);
}

void VkDDRenderInterface::setupTextUniforms()
{
	d_text.screenDimensions.x = d_viewport.width;
	d_text.screenDimensions.y = d_viewport.height;

	vk::BufferCreateInfo sbo_create_info = {};
	VmaAllocationCreateInfo sbo_alloc_info = {};
	sbo_create_info.size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex);
	sbo_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	sbo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	d_text.ubo = d_vkCtx->createSharedBufferObject(sbo_create_info, sbo_alloc_info);
	d_vkCtx->upload(*d_text.ubo, glm::value_ptr(d_text.screenDimensions), sizeof(glm::vec2));

	d_text.sampler =
		d_vkCtx->vkDevice().createSampler(vk::SamplerCreateInfo(
		vk::SamplerCreateFlags(),
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge
		));

	d_text.bindings = {
		vk::DescriptorSetLayoutBinding(
			0, vk::DescriptorType::eUniformBuffer,
			1, vk::ShaderStageFlagBits::eVertex
		),
		vk::DescriptorSetLayoutBinding(
			1, vk::DescriptorType::eCombinedImageSampler,
			1,  vk::ShaderStageFlagBits::eFragment
		//&d_text.sampler
	)
	};

	d_text.descriptorSetLayout =
		d_vkCtx->vkDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		static_cast<uint32_t>(d_text.bindings.size()),
		d_text.bindings.data()
		));

	d_text.descriptorSet =
		d_vkCtx->vkDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo(
		d_vkCtx->vkDescriptorPool(),
		1, &d_text.descriptorSetLayout
		))[0];

	d_text.pipelineLayout =
		d_vkCtx->vkDevice().createPipelineLayout(vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		1, &d_text.descriptorSetLayout
		));


	d_text.descriptorBufferInfo.buffer = d_text.ubo->buffer;
	d_text.descriptorBufferInfo.offset = 0;
	d_text.descriptorBufferInfo.range = sizeof(glm::vec2);

	d_text.descriptorImageInfo.sampler = d_text.sampler;
	d_text.descriptorImageInfo.imageView = d_text.imageView;
	d_text.descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	// TODO:
	d_text.writeDescriptorSets = {
		vk::WriteDescriptorSet(d_text.descriptorSet, 0, 0, 1,
			vk::DescriptorType::eUniformBuffer, nullptr, &d_text.descriptorBufferInfo
		),
		vk::WriteDescriptorSet(d_text.descriptorSet, 1, 0, 1,
			vk::DescriptorType::eCombinedImageSampler, &d_text.descriptorImageInfo, nullptr
		)
	};

	d_vkCtx->vkDevice().updateDescriptorSets(d_text.writeDescriptorSets, nullptr);

	// texture pipeline
	d_textPipeline.pipeline =
		d_vkCtx->vkDevice().createGraphicsPipeline(d_textPipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_textPipeline.pipelineShaderStages.size()),
		d_textPipeline.pipelineShaderStages.data(),
		&d_text_vertices.inputState,
		&d_textPipeline.inputAssemblyState,
		nullptr,
		&d_textPipeline.viewportState,
		&d_textPipeline.rasterState,
		&d_textPipeline.multisampleState,
		&d_textPipeline.depthStencilState,
		&d_textPipeline.colorBlendState,
		&d_textPipeline.dynamicState,
		d_text.pipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
		)
		);
}

void VkDDRenderInterface::setupPipelines()
{
	std::printf("> DDRenderInterfaceCoreGL::setupPipelines()\n");

	d_linePointPipeline.vert = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "line_point.vert.spv"
	);

	d_linePointPipeline.frag = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "line_point.frag.spv"
	);

	d_textPipeline.vert = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "dd_text.vert.spv"
	);

	d_textPipeline.frag = d_vkCtx->createShaderModule(
		app::SystemMgr::instance().settings().shader_dir + "dd_text.frag.spv"
	);


	d_linePointPipeline.pipelineCache = d_vkCtx->vkDevice().createPipelineCache(vk::PipelineCacheCreateInfo());
	d_textPipeline.pipelineCache = d_vkCtx->vkDevice().createPipelineCache(vk::PipelineCacheCreateInfo());

	d_linePointPipeline.pipelineShaderStages = {
	vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		d_linePointPipeline.vert,
		"main",
		nullptr
	),
	vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eFragment,
		d_linePointPipeline.frag,
		"main",
		nullptr
	)
	};

	d_textPipeline.pipelineShaderStages = {
	vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		d_textPipeline.vert,
		"main",
		nullptr
	),

	vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eFragment,
		d_textPipeline.frag,
		"main",
		nullptr
	)
	};

	/////////////////////////
	d_linePointPipeline.inputAssemblyStatePoint = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::ePointList
	);

	d_linePointPipeline.inputAssemblyStateLine = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eLineList
	);

	d_textPipeline.inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList
	);

	/////////////////////////
	d_viewport = d_vkCtx->viewport();
	d_renderArea = d_vkCtx->renderArea();
	d_linePointPipeline.viewportState = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlagBits(),
		1, &d_viewport,
		1, &d_renderArea
	);

	d_textPipeline.viewportState = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlagBits(),
		1, &d_viewport,
		1, &d_renderArea
	);

	//////////////////////////
	d_linePointPipeline.rasterState = vk::PipelineRasterizationStateCreateInfo(
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

	d_textPipeline.rasterState = vk::PipelineRasterizationStateCreateInfo(
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

	/////////////////////
	d_linePointPipeline.multisampleState = vk::PipelineMultisampleStateCreateInfo(
		vk::PipelineMultisampleStateCreateFlags(),
		vk::SampleCountFlagBits::e1
	);

	d_textPipeline.multisampleState = vk::PipelineMultisampleStateCreateInfo(
		vk::PipelineMultisampleStateCreateFlags(),
		vk::SampleCountFlagBits::e1
	);

	//////////////////
	d_linePointPipeline.depthStencilStateDepthTestEnabled = vk::PipelineDepthStencilStateCreateInfo(
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
	d_linePointPipeline.depthStencilStateDepthTestDisabled = vk::PipelineDepthStencilStateCreateInfo(
		vk::PipelineDepthStencilStateCreateFlags(),
		VK_FALSE,
		VK_TRUE,
		vk::CompareOp::eLessOrEqual,
		VK_FALSE,
		VK_FALSE,
		vk::StencilOpState(),
		vk::StencilOpState(),
		0,
		0
	);


	d_textPipeline.depthStencilState = vk::PipelineDepthStencilStateCreateInfo(
		vk::PipelineDepthStencilStateCreateFlags(),
		VK_FALSE,
		VK_TRUE,
		vk::CompareOp::eLessOrEqual,
		VK_FALSE,
		VK_FALSE,
		vk::StencilOpState(),
		vk::StencilOpState(),
		0,
		0
	);

	/////////////////////////////
	d_linePointPipeline.colorBlendAttachments = {
		vk::PipelineColorBlendAttachmentState(
			VK_FALSE,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eOne,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		)
	};

	// TODO:
	d_textPipeline.colorBlendAttachments = {
		vk::PipelineColorBlendAttachmentState(
			VK_TRUE,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		)
	};


	d_linePointPipeline.colorBlendState = vk::PipelineColorBlendStateCreateInfo(
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eClear,
		static_cast<uint32_t>(d_linePointPipeline.colorBlendAttachments.size()),
		d_linePointPipeline.colorBlendAttachments.data()
	);

	d_textPipeline.colorBlendState = vk::PipelineColorBlendStateCreateInfo(
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eClear,
		static_cast<uint32_t>(d_textPipeline.colorBlendAttachments.size()),
		d_textPipeline.colorBlendAttachments.data()
	);

	/////////////////////////////////
	d_linePointPipeline.dynamicStateList = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		//vk::DynamicState::eLineWidth
	};

	d_textPipeline.dynamicStateList = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	d_linePointPipeline.dynamicState = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		static_cast<uint32_t>(d_linePointPipeline.dynamicStateList.size()),
		d_linePointPipeline.dynamicStateList.data()
	);

	d_textPipeline.dynamicState = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		static_cast<uint32_t>(d_textPipeline.dynamicStateList.size()),
		d_textPipeline.dynamicStateList.data()
	);

	//////////////////////
	auto linepointPipelineLayout = d_mvp.pipelineLayout;

	d_linePointPipeline.pipelineHandle_depthTestEnabled_point =
		d_vkCtx->vkDevice().createGraphicsPipeline(d_linePointPipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_linePointPipeline.pipelineShaderStages.size()),
		d_linePointPipeline.pipelineShaderStages.data(),
		&d_vertices.inputState,
		&d_linePointPipeline.inputAssemblyStatePoint,
		nullptr,
		&d_linePointPipeline.viewportState,
		&d_linePointPipeline.rasterState,
		&d_linePointPipeline.multisampleState,
		&d_linePointPipeline.depthStencilStateDepthTestEnabled,
		&d_linePointPipeline.colorBlendState,
		&d_linePointPipeline.dynamicState,
		linepointPipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
		)
		);

	d_linePointPipeline.pipelineHandle_depthTestDisabled_point =
		d_vkCtx->vkDevice().createGraphicsPipeline(d_linePointPipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_linePointPipeline.pipelineShaderStages.size()),
		d_linePointPipeline.pipelineShaderStages.data(),
		&d_vertices.inputState,
		&d_linePointPipeline.inputAssemblyStatePoint,
		nullptr,
		&d_linePointPipeline.viewportState,
		&d_linePointPipeline.rasterState,
		&d_linePointPipeline.multisampleState,
		&d_linePointPipeline.depthStencilStateDepthTestDisabled,
		&d_linePointPipeline.colorBlendState,
		&d_linePointPipeline.dynamicState,
		linepointPipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
		)
		);

	d_linePointPipeline.pipelineHandle_depthTestEnabled_line =
		d_vkCtx->vkDevice().createGraphicsPipeline(d_linePointPipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_linePointPipeline.pipelineShaderStages.size()),
		d_linePointPipeline.pipelineShaderStages.data(),
		&d_vertices.inputState,
		&d_linePointPipeline.inputAssemblyStateLine,
		nullptr,
		&d_linePointPipeline.viewportState,
		&d_linePointPipeline.rasterState,
		&d_linePointPipeline.multisampleState,
		&d_linePointPipeline.depthStencilStateDepthTestEnabled,
		&d_linePointPipeline.colorBlendState,
		&d_linePointPipeline.dynamicState,
		linepointPipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
		)
		);

	d_linePointPipeline.pipelineHandle_depthTestDisabled_line =
		d_vkCtx->vkDevice().createGraphicsPipeline(d_linePointPipeline.pipelineCache,
		vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(d_linePointPipeline.pipelineShaderStages.size()),
		d_linePointPipeline.pipelineShaderStages.data(),
		&d_vertices.inputState,
		&d_linePointPipeline.inputAssemblyStateLine,
		nullptr,
		&d_linePointPipeline.viewportState,
		&d_linePointPipeline.rasterState,
		&d_linePointPipeline.multisampleState,
		&d_linePointPipeline.depthStencilStateDepthTestDisabled,
		&d_linePointPipeline.colorBlendState,
		&d_linePointPipeline.dynamicState,
		linepointPipelineLayout,
		d_vkCtx->defaultRenderPass(),
		0
		)
		);
}


} // end namespace dd