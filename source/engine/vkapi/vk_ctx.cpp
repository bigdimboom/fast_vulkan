#include "vk_ctx.h"
#include <map>
#include <fstream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace vkapi
{
//HELPERS
VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	fprintf(stderr, "[vulkan] ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	return VK_FALSE;
}

void findBestExtensions(const std::vector<vk::ExtensionProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out)
{
	for (const char* const& w : wanted) {
		for (vk::ExtensionProperties const& i : installed) {
			if (std::string(i.extensionName).compare(w) == 0) {
				out.emplace_back(w);
				break;
			}
		}
	}
}

void findBestLayers(const std::vector<vk::LayerProperties>& installed, const std::vector<const char*>& wanted, std::vector<const char*>& out)
{
	for (const char* const& w : wanted) {
		for (vk::LayerProperties const& i : installed) {
			if (std::string(i.layerName).compare(w) == 0) {
				out.emplace_back(w);
				break;
			}
		}
	}
}

uint32_t getQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlags flags, bool standalone)
{
	std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();

	if (!standalone)
	{
		for (size_t i = 0; i < queueProps.size(); ++i)
		{
			if (queueProps[i].queueFlags & flags) {
				return static_cast<uint32_t>(i);
			}
		}
	}
	else
	{
		for (size_t i = 0; i < queueProps.size(); ++i)
		{
			if (queueProps[i].queueFlags == flags) {
				return static_cast<uint32_t>(i);
			}
		}
	}

	return 0;
}

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

// MEMBERS
Context::Context(window::VKWindow& vkWindow, const CtxSettings& settings)
	: d_window(vkWindow)
	, d_settings(settings)
{
	setupVulkanInstance();
	setupPhsicalDevice();
	setupLogicalDevice();
	setupMemoryAllocator();
	setupSurface();
	setupSwapchain();
	setupRenderPass();
	setupFrameBuffer();
	setupDrawCommandsAndSynchronization();
	setupDescriptorPool();
}

Context::~Context()
{
	d_logical_device.waitIdle();

	d_logical_device.destroyDescriptorPool(d_descriptorPool);

	//removeAllStaticDraws();

	destroyDrawCommandsAndSynchronization();

	destroyFrameBuffers();

	destroyRenderpass();

	d_logical_device.destroySwapchainKHR(d_swapchain);

	d_instance.destroySurfaceKHR(d_surface);

	for (auto& elem : d_queues)
	{
		d_logical_device.destroyCommandPool(elem.cmdPools);
	}

	vmaDestroyAllocator(d_allocator);

	d_logical_device.destroy();

	if (d_debugReport)
	{
		d_Dldy.vkDestroyDebugReportCallbackEXT(d_instance, d_debugReport, nullptr);
	}

	d_instance.destroy();
}

vk::Instance Context::vkInstance() const
{
	return d_instance;
}

vk::PhysicalDevice Context::vkPhysicalDevice() const
{
	return d_physcial_device;
}

vk::Device Context::vkDevice() const
{
	return d_logical_device;
}

vk::SurfaceKHR Context::vkSurface() const
{
	return d_surface;
}

vk::SwapchainKHR Context::vkSwapchain() const
{
	return d_swapchain;
}

vk::DescriptorPool Context::vkDescriptorPool() const
{
	return d_descriptorPool;
}

vk::RenderPass Context::defaultRenderPass() const
{
	return d_renderPass;
}

vk::CommandBuffer Context::commandBuffer() const
{
	return d_commandBuffers[d_frameIndex];
}

vk::Viewport Context::viewport() const
{
	return d_viewport;
}

vk::Rect2D Context::renderArea() const
{
	return d_renderArea;
}

VmaAllocator Context::memAllocator() const
{
	return d_allocator;
}

vk::Queue Context::vkQueue(vk::QueueFlagBits flag) const
{
	switch (flag)
	{
	case vk::QueueFlagBits::eGraphics:
		return d_queues[graphics].vkQueue;
	case vk::QueueFlagBits::eCompute:
		return d_queues[compute].vkQueue;
	case vk::QueueFlagBits::eTransfer:
		return d_queues[transfer].vkQueue;
	}
	assert(0);
	return vk::Queue();
}

vk::CommandPool Context::vkCmdPool(vk::QueueFlagBits flag) const
{
	switch (flag)
	{
	case vk::QueueFlagBits::eGraphics:
		return d_queues[graphics].cmdPools;
	case vk::QueueFlagBits::eCompute:
		return d_queues[compute].cmdPools;
	case vk::QueueFlagBits::eTransfer:
		return d_queues[transfer].cmdPools;
	}
	assert(0);
	return vk::CommandPool();
}

uint32_t Context::familyQueueIndex(vk::QueueFlagBits flag) const
{
	switch (flag)
	{
	case vk::QueueFlagBits::eGraphics:
		return d_queues[graphics].familyQueueIndex;
	case vk::QueueFlagBits::eCompute:
		return d_queues[compute].familyQueueIndex;
	case vk::QueueFlagBits::eTransfer:
		return d_queues[transfer].familyQueueIndex;
	}
	assert(0);
	return 0;
}

uint32_t Context::nSwapchainFrameBuffers() const
{
	return static_cast<uint32_t>(d_swapchainFrameBuffers.size());
}

void Context::check_error(vk::Result result)
{
	if (result != vk::Result::eSuccess)
	{
		SDL_Log("vulkan: error %s", vk::to_string(result).c_str());
	}
}

void Context::check_error(VkResult result)
{
	check_error(vk::Result(result));
}

void Context::resize()
{
	d_logical_device.waitIdle();

	destroyDrawCommandsAndSynchronization();
	destroyFrameBuffers();
	destroyRenderpass();

	setupSwapchain();
	setupRenderPass();
	setupFrameBuffer();
	setupDrawCommandsAndSynchronization();
}

void Context::setClearValue(float r, float g, float b, float a)
{
	d_clearValues[0] = vk::ClearColorValue(std::array<float, 4>{ r, g, b, a });
	d_clearValues[1] = vk::ClearDepthStencilValue(1.0f, 0u);
}

void Context::setClearValues(vk::ClearColorValue color, vk::ClearDepthStencilValue depth)
{
	d_clearValues[0] = color;
	d_clearValues[1] = depth;
}

void Context::frameBegin()
{
	vk::Result result = d_logical_device.acquireNextImageKHR(d_swapchain, UINT64_MAX, d_presentCompleteSemaphores[d_frameIndex], nullptr, &d_frameIndex);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		// Swapchain lost, we'll try again next poll
		resize();
		return;

	}
	if (result == vk::Result::eErrorDeviceLost)
	{
		// driver lost, we'll crash in this case:
		check_error(result);
		exit(1);
	}

	d_logical_device.waitForFences(1, &d_waitFences[d_frameIndex], VK_TRUE, UINT64_MAX);
	d_logical_device.resetFences(1, &d_waitFences[d_frameIndex]);
	d_commandBuffers[d_frameIndex].reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	d_commandBuffers[d_frameIndex].begin(vk::CommandBufferBeginInfo());
}

void Context::beginDefaultRenderPass()
{
	d_commandBuffers[d_frameIndex].beginRenderPass(
		vk::RenderPassBeginInfo(
		d_renderPass,
		d_swapchainFrameBuffers[d_frameIndex].frameBuffer,
		d_renderArea,
		static_cast<uint32_t>(d_clearValues.size()),
		d_clearValues.data()),
		vk::SubpassContents::eInline);
}

//void Context::flushStaticDraws()
//{
//	for (auto& elem : d_staticDraws)
//	{
//		d_commandBuffers[d_frameIndex].executeCommands(elem.second);
//	}
//}


void Context::endDefaultRenderPass()
{
	d_commandBuffers[d_frameIndex].endRenderPass();
}

void Context::frameEnd()
{
	d_commandBuffers[d_frameIndex].end();

	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&d_presentCompleteSemaphores[d_frameIndex])
		.setPWaitDstStageMask(&waitDstStageMask)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&d_commandBuffers[d_frameIndex])
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&d_renderCompleteSemaphores[d_frameIndex]);

	auto result = d_queues[graphics].vkQueue.submit(1, &submitInfo, d_waitFences[d_frameIndex]);

	if (result == vk::Result::eErrorDeviceLost)
	{
		// driver lost, we'll crash in this case:
		exit(1);
	}
}

void Context::framePresent()
{
	auto result = d_queues[graphics].vkQueue.presentKHR(
		vk::PresentInfoKHR(
		1,
		&d_renderCompleteSemaphores[d_frameIndex],
		1,
		&d_swapchain,
		&d_frameIndex,
		nullptr
	)
	);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		// Swapchain lost, we'll try again next poll
		resize();
		return;
	}

	d_frameIndex = (d_frameIndex + 1) % d_settings.backbuffer_count;
}

vk::CommandBuffer Context::beginSingleTimeCommands(bool begin)
{
	vk::CommandBuffer cmdBuffer = d_logical_device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo(
		d_queues[graphics].cmdPools,
		vk::CommandBufferLevel::ePrimary,
		1)
	)[0];

	// If requested, also start the new command buffer
	if (begin)
	{
		cmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	}

	return cmdBuffer;
}

void Context::flushSingleTimeCommands(vk::CommandBuffer& commandBuffer, bool end)
{
	if (end)
	{
		commandBuffer.end();
	}

	std::vector<vk::SubmitInfo> submitInfos = {
		vk::SubmitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr)
	};

	// Create fence to ensure that the command buffer has finished executing
	vk::Fence fence = d_logical_device.createFence(vk::FenceCreateInfo());

	// Submit to the queue
	d_queues[graphics].vkQueue.submit(submitInfos, fence);
	// Wait for the fence to signal that command buffer has finished executing
	d_logical_device.waitForFences(1, &fence, VK_TRUE, UINT_MAX);
	d_logical_device.destroyFence(fence);
	d_logical_device.freeCommandBuffers(d_queues[graphics].cmdPools, 1, &commandBuffer);
}

//void Context::addStaticDraw(std::function<void(vk::CommandBuffer, vk::RenderPass)> func, const char* key, vk::RenderPass renderpass)
//{
//	if (renderpass == vk::RenderPass(nullptr))
//	{
//		renderpass = defaultRenderPass();
//	}
//
//	auto secondaryCommands = d_logical_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
//		d_queues[graphics].cmdPools,
//		vk::CommandBufferLevel::eSecondary,
//		d_commandBuffers.size()
//	));
//
//	for (int i = 0; i < secondaryCommands.size(); ++i)
//	{
//		auto cmd = secondaryCommands[i];
//		cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
//		vk::CommandBufferInheritanceInfo inheritInfo;
//		inheritInfo.framebuffer = d_swapchainFrameBuffers[i].frameBuffer;
//		inheritInfo.renderPass = renderpass;
//		cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eSimultaneousUse, &inheritInfo));
//		func(cmd, renderpass);
//		cmd.end();
//	}
//
//	d_staticDraws[key] = secondaryCommands;
//}
//
//void Context::removeStaticDraw(const char* key)
//{
//	assert(d_staticDraws.count(key) > 0);
//	d_logical_device.freeCommandBuffers(d_queues[graphics].cmdPools, d_staticDraws[key]);
//	d_staticDraws.erase(key);
//}
//
//void Context::removeAllStaticDraws()
//{
//	for (auto& elem : d_staticDraws)
//	{
//		d_logical_device.freeCommandBuffers(d_queues[graphics].cmdPools, elem.second);
//		elem.second.clear();
//	}
//	d_staticDraws.clear();
//}

VkShaderModule Context::createShaderModule(const std::vector<char>& code)
{
	return d_logical_device.createShaderModule(
		vk::ShaderModuleCreateInfo(
		vk::ShaderModuleCreateFlags(),
		code.size(),
		reinterpret_cast<const uint32_t*>(code.data())
	));
}

VkShaderModule Context::createShaderModule(const std::string& filename)
{
	return createShaderModule(readFile(filename));
}

void Context::copy(vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyBuffer(src, dst, region);
	flushSingleTimeCommands(cmd, true);
}

void Context::copy(vk::Image dst, vk::Buffer src, const vk::BufferImageCopy& region, vk::ImageLayout layout)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyBufferToImage(src, dst, layout, region);
	flushSingleTimeCommands(cmd, true);
}

void Context::copy(vk::Buffer dst, vk::Image src, const vk::BufferImageCopy& region, vk::ImageLayout layout)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyImageToBuffer(src, layout, dst, region);
	flushSingleTimeCommands(cmd, true);
}

void Context::upload(BufferObject& dst_hostVisable, void* src_host, size_t size_bytes, size_t dst_offset)
{
	void* dst = nullptr;
	check_error(vmaMapMemory(d_allocator, dst_hostVisable.alloc_meta, &dst));
	memcpy((uint8_t*)dst + dst_offset, src_host, size_bytes);
	vmaUnmapMemory(d_allocator, dst_hostVisable.alloc_meta);
}

void Context::copy(vk::Buffer dst, vk::Buffer src, const std::vector<vk::BufferCopy>& regions)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyBuffer(src, dst, regions);
	flushSingleTimeCommands(cmd, true);
}

void Context::copy(vk::Image dst, vk::Buffer src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyBufferToImage(src, dst, layout, regions);
	flushSingleTimeCommands(cmd, true);
}

void Context::copy(vk::Buffer dst, vk::Image src, const std::vector<vk::BufferImageCopy>& regions, vk::ImageLayout layout)
{
	auto cmd = beginSingleTimeCommands(true);
	cmd.copyImageToBuffer(src, layout, dst, regions);
	flushSingleTimeCommands(cmd, true);
}

std::shared_ptr<BufferObject> Context::createSharedBufferObject(const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info)
{
	auto data = std::shared_ptr<BufferObject>(new BufferObject, [this](BufferObject* obj) {vmaDestroyBuffer(d_allocator, obj->buffer, obj->alloc_meta); });
	check_error(vmaCreateBuffer(d_allocator, (VkBufferCreateInfo*)& bufferinfo, &alloc_info, (VkBuffer*)& data->buffer, &data->alloc_meta, nullptr));
	return data;
}

std::shared_ptr<ImageObject> Context::createSharedImageObject(const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info)
{
	auto ret = std::shared_ptr<ImageObject>(new ImageObject, [this](ImageObject* obj)
	{
		vmaDestroyImage(d_allocator, obj->image, obj->alloc_meta);
	});

	check_error(vmaCreateImage(d_allocator, (VkImageCreateInfo*)& image_info, &alloc_info, (VkImage*)& ret->image, &ret->alloc_meta, nullptr));
	return ret;
}

std::shared_ptr<ImageObject> Context::createTextureImage2D(int width, int height, vk::Format format, void* hostdata_ptr)
{
	uint32_t channels = 0;
	uint32_t elem_size = 0;

	switch (format)
	{
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eR8G8B8A8Unorm:
		channels = 4;
		elem_size = 8 / 8;
		break;
	case vk::Format::eB8G8R8Unorm:
	case vk::Format::eR8G8B8Unorm:
		channels = 3;
		elem_size = 8 / 8;
		break;
	case vk::Format::eR8Unorm:
		channels = 1;
		elem_size = 8 / 8;
		break;
	case vk::Format::eD16Unorm:
		channels = 1;
		elem_size = 16 / 8;
		break;
	case vk::Format::eR32G32B32A32Sfloat:
		channels = 4;
		elem_size = 32 / 8;
		break;
	case vk::Format::eR32G32B32Sfloat:
		channels = 3;
		elem_size = 32 / 8;
		break;
	case vk::Format::eR32Sfloat:
		channels = 1;
		elem_size = 32 / 8;
		break;
	default:
		SDL_Log("unsupported type: %s", vk::to_string(format).c_str());
		throw std::invalid_argument("unsupported fomat type!");
		assert(0);
	}

	vk::BufferCreateInfo stagingBufferInfo = {};
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingBufferInfo.size = width * height * channels * elem_size;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	auto stagebuffer = createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
	upload(*stagebuffer, hostdata_ptr, width * height * channels * elem_size);

	VmaAllocationCreateInfo image_alloc_info = {};
	image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	auto familyQueueIndex = this->familyQueueIndex();
	auto texture = this->createSharedImageObject(vk::ImageCreateInfo(
		vk::ImageCreateFlags(),
		vk::ImageType::e2D,
		format,
		vk::Extent3D(vk::Extent2D(width, height), 1),
		1u, 1u,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive,
		1,
		&familyQueueIndex,
		vk::ImageLayout::eUndefined),
		image_alloc_info);


	vk::BufferImageCopy copyParams(
		0u, 0u, 0u,
		vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1),
		vk::Offset3D(0, 0, 0),
		vk::Extent3D(width, height, 1));

	transitionImageLayout(texture->image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copy(texture->image, stagebuffer->buffer, copyParams);
	transitionImageLayout(texture->image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	return texture;
}

void Context::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::ImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	transitionImageLayout(image, format, oldLayout, newLayout, subresourceRange);
}

void Context::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageSubresourceRange subresourceRange)
{
	auto cmd = beginSingleTimeCommands(true);
	vk::ImageMemoryBarrier barrier = {};
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange = subresourceRange;
	barrier.srcAccessMask = vk::AccessFlags();  //
	barrier.dstAccessMask = vk::AccessFlags();  //

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	// TODO:
	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		if (hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	if (oldLayout == vk::ImageLayout::eUndefined &&
		(newLayout == vk::ImageLayout::eTransferDstOptimal ||
		newLayout == vk::ImageLayout::eTransferSrcOptimal))
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
		newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined &&
		newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask =
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
	{
		SDL_Log("unsupported layouts: %s, %s",
			vk::to_string(oldLayout).c_str(),
			vk::to_string(newLayout).c_str());

		throw std::invalid_argument("unsupported layout transition!");
		assert(0);
	}

	cmd.pipelineBarrier(sourceStage, destinationStage,
		vk::DependencyFlags(),
		{},
		{},
		barrier);

	flushSingleTimeCommands(cmd, true);
}

std::shared_ptr<BufferObject> Context::createUniformBufferObject(uint64_t size)
{
	vk::BufferCreateInfo sbo_create_info = {};
	VmaAllocationCreateInfo sbo_alloc_info = {};
	sbo_create_info.size = size;
	sbo_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	sbo_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	return createSharedBufferObject(sbo_create_info, sbo_alloc_info);
}

std::shared_ptr<BufferObject> Context::createStagingBufferObject(uint64_t size)
{
	vk::BufferCreateInfo stagingBufferInfo = {};
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingBufferInfo.size = size;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	return createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
}

// HELPERS
void Context::setupVulkanInstance()
{
	auto wantedExtensions = d_window.vkInstanceExtensions();
	auto wantedLayers = std::vector<const char*>();

	if (d_settings.debug)
	{
		wantedLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		wantedExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	// Find the best Instance Extensions
	auto installedExtensions = vk::enumerateInstanceExtensionProperties();
	std::vector<const char*> extensions = {};
	findBestExtensions(installedExtensions, wantedExtensions, extensions);
	// find best instance extensions
	auto installedLayers = vk::enumerateInstanceLayerProperties();
	std::vector<const char*> layers = {};
	findBestLayers(installedLayers, wantedLayers, layers);

	vk::ApplicationInfo appInfo(
		d_settings.appname.c_str(), 0,
		d_settings.enginename.c_str(), 0,
		VK_API_VERSION_1_0
	);

	vk::InstanceCreateInfo info(
		vk::InstanceCreateFlags(),
		&appInfo,
		static_cast<uint32_t>(layers.size()),
		layers.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data()
	);

	d_instance = vk::createInstance(info);

	if (d_settings.debug)
	{
		d_Dldy.init(d_instance);
		VkDebugReportCallbackEXT handle;

		// Setup the debug report callback
		VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
		debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_ci.pfnCallback = debug_report;
		debug_report_ci.pUserData = NULL;

		auto creat_func = d_Dldy.vkCreateDebugReportCallbackEXT;

		if (creat_func)
		{
			creat_func(d_instance, &debug_report_ci, nullptr, &handle);
			d_debugReport = vk::DebugReportCallbackEXT(handle);
		}
		else
		{
			SDL_Log("%s library is not there. VkDebug is not enabled", VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
	}
}

void Context::setupPhsicalDevice()
{
	std::vector<vk::PhysicalDevice> physicalDevices =
		d_instance.enumeratePhysicalDevices();

	auto id = d_settings.gpuid;

	if (id < 0 || id >= physicalDevices.size())
	{
		SDL_Log("GPU ID error.");
		exit(EXIT_FAILURE);
	}

	d_physcial_device = physicalDevices[id];
	d_queues[graphics].familyQueueIndex = getQueueIndex(d_physcial_device,
		vk::QueueFlagBits::eGraphics |
		vk::QueueFlagBits::eCompute |
		vk::QueueFlagBits::eTransfer,
		false);

	if (d_settings.standalone_compute_queue)
	{
		d_queues[compute].familyQueueIndex = getQueueIndex(d_physcial_device,
			vk::QueueFlagBits::eCompute,
			true);
	}
	else
	{
		d_queues[compute].familyQueueIndex = d_queues[graphics].familyQueueIndex;
	}

	if (d_settings.standalone_transfer_queue)
	{
		d_queues[transfer].familyQueueIndex = getQueueIndex(d_physcial_device,
			vk::QueueFlagBits::eTransfer,
			true);
	}
	else
	{
		d_queues[transfer].familyQueueIndex = d_queues[graphics].familyQueueIndex;
	}

}

void Context::setupLogicalDevice()
{
	std::map<uint32_t, uint32_t> counts;
	counts[d_queues[graphics].familyQueueIndex]++;
	if (d_settings.standalone_compute_queue)
	{
		counts[d_queues[compute].familyQueueIndex]++;
	}
	if (d_settings.standalone_transfer_queue)
	{
		counts[d_queues[transfer].familyQueueIndex]++;
	}

	std::vector<vk::DeviceQueueCreateInfo> qcinfo;
	for (const auto& elem : counts)
	{
		if (elem.second > 0)
		{
			qcinfo.push_back({});
			qcinfo.back().setQueueFamilyIndex(elem.first);
			qcinfo.back().setQueueCount(elem.second);
			auto mQueuePriority = 0.5f;
			qcinfo.back().setPQueuePriorities(&mQueuePriority);
		}
	}


	// Logical Context
	std::vector<vk::ExtensionProperties> installedDeviceExtensions =
		d_physcial_device.enumerateDeviceExtensionProperties();

	std::vector<const char*> wantedDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::vector<const char*> deviceExtensions = {};

	findBestExtensions(installedDeviceExtensions, wantedDeviceExtensions, deviceExtensions);

	vk::DeviceCreateInfo dinfo;
	dinfo.setPQueueCreateInfos(qcinfo.data());
	dinfo.setQueueCreateInfoCount(static_cast<uint32_t>(qcinfo.size()));
	dinfo.setPpEnabledExtensionNames(deviceExtensions.data());
	dinfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
	d_logical_device = d_physcial_device.createDevice(dinfo);

	for (int i = 0; i < QueueType::QueueTypeSize; ++i)
	{
		d_queues[i].vkQueue = d_logical_device.getQueue(d_queues[i].familyQueueIndex, 0);
		d_queues[i].cmdPools = d_logical_device.createCommandPool(
			vk::CommandPoolCreateInfo(
			vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
			d_queues[i].familyQueueIndex
		));
	}
}

void Context::setupSurface()
{
	// Surface
	d_surface = d_window.createSurface(d_instance);
	if (!d_physcial_device.getSurfaceSupportKHR(d_queues[graphics].familyQueueIndex, d_surface))
	{
		// Check if queueFamily supports this surface
		return;
	}
}

void Context::setupSwapchain()
{
	int width = d_window.pixelrez().x;
	int height = d_window.pixelrez().y;

	// Setup viewports, Vsync
	vk::Extent2D swapchainSize = vk::Extent2D(width, height);

	// All framebuffers / attachments will be the same size as the surface
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = d_physcial_device.getSurfaceCapabilitiesKHR(d_surface);
	if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
		swapchainSize = surfaceCapabilities.currentExtent;
		d_renderArea = vk::Rect2D(vk::Offset2D(), swapchainSize);
		d_viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainSize.width), static_cast<float>(swapchainSize.height), 0, 1.0f);
	}

	// VSync
	std::vector<vk::PresentModeKHR> surfacePresentModes = d_physcial_device.getSurfacePresentModesKHR(d_surface);
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

	if (d_settings.backbuffer_count > 1)
	{
		for (vk::PresentModeKHR& pm : surfacePresentModes) {
			if (pm == vk::PresentModeKHR::eMailbox) {
				presentMode = vk::PresentModeKHR::eMailbox;
				break;
			}
		}
	}

	if (d_settings.vsync)
	{
		for (vk::PresentModeKHR& pm : surfacePresentModes) {
			if (pm == vk::PresentModeKHR::eFifo) {
				presentMode = vk::PresentModeKHR::eFifo;
				break;
			}
		}
	}

	// Create Swapchain, Images, Frame Buffers

	d_logical_device.waitIdle();
	vk::SwapchainKHR oldSwapchain = d_swapchain;

	// Some devices can support more than 2 buffers, but during my tests they would crash on fullscreen ~ ag
	// Tested on an NVIDIA 1080 and 165 Hz 2K display
	uint32_t backbufferCount = glm::clamp(d_settings.backbuffer_count, 2u, surfaceCapabilities.maxImageCount);
	d_settings.backbuffer_count = backbufferCount;

	checkSurfaceFormat();

	d_swapchain = d_logical_device.createSwapchainKHR(
		vk::SwapchainCreateInfoKHR(
		vk::SwapchainCreateFlagsKHR(),
		d_surface,
		backbufferCount,
		d_surfaceColorFormat,
		d_surfaceColorSpace,
		swapchainSize,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		1,
		&d_queues[graphics].familyQueueIndex,
		vk::SurfaceTransformFlagBitsKHR::eIdentity,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode,
		VK_TRUE,
		oldSwapchain
	)
	);

	d_surfaceSize = vk::Extent2D(glm::clamp(swapchainSize.width, 1U, 8192U), glm::clamp(swapchainSize.height, 1U, 8192U));
	d_renderArea = vk::Rect2D(vk::Offset2D(), d_surfaceSize);
	d_viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(d_surfaceSize.width), static_cast<float>(d_surfaceSize.height), 0, 1.0f);


	// Destroy previous swapchain
	if (oldSwapchain != vk::SwapchainKHR(nullptr))
	{
		d_logical_device.destroySwapchainKHR(oldSwapchain);
	}

	d_frameIndex = 0;
}

void Context::checkSurfaceFormat()
{
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = d_physcial_device.getSurfaceFormatsKHR(d_surface);

	for (const auto& elem : surfaceFormats)
	{
		if (elem.format == d_surfaceColorFormat && elem.colorSpace == d_surfaceColorSpace)
		{
			return;
		}
	}

	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
	{
		d_surfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
	}
	else
	{
		d_surfaceColorFormat = surfaceFormats[0].format;
	}

	d_surfaceColorSpace = surfaceFormats[0].colorSpace;
}

void Context::setupRenderPass()
{
	std::vector<vk::AttachmentDescription> attachmentDescriptions =
	{
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(),
			d_surfaceColorFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		),
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(),
			d_surfaceDepthFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		)
	};

	std::vector<vk::AttachmentReference> colorReferences =
	{
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)
	};

	std::vector<vk::AttachmentReference> depthReferences = {
		vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
	};

	std::vector<vk::SubpassDescription> subpasses =
	{
		vk::SubpassDescription(
			vk::SubpassDescriptionFlags(),
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			static_cast<uint32_t>(colorReferences.size()),
			colorReferences.data(),
			nullptr,
			depthReferences.data(),
			0,
			nullptr
		)
	};

	std::vector<vk::SubpassDependency> dependencies =
	{
		vk::SubpassDependency(
			~0U,
			0,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eMemoryRead,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		),
		vk::SubpassDependency(
			0,
			~0U,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::DependencyFlagBits::eByRegion
		)
	};

	d_renderPass = d_logical_device.createRenderPass(
		vk::RenderPassCreateInfo(
		vk::RenderPassCreateFlags(),
		static_cast<uint32_t>(attachmentDescriptions.size()),
		attachmentDescriptions.data(),
		static_cast<uint32_t>(subpasses.size()),
		subpasses.data(),
		static_cast<uint32_t>(dependencies.size()),
		dependencies.data()
	)
	);

	setClearValue(0.0f, 0.0f, 0.0f, 1.0f);
}

void Context::setupFrameBuffer()
{
	d_swapchainFrameBuffers.resize(d_settings.backbuffer_count);

	vk::ImageCreateInfo image_ci(
		vk::ImageCreateFlags(),
		vk::ImageType::e2D,
		d_surfaceDepthFormat,
		vk::Extent3D(d_surfaceSize.width, d_surfaceSize.height, 1),
		1U,
		1U,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
		vk::SharingMode::eExclusive,
		1,
		&d_queues[graphics].familyQueueIndex,
		vk::ImageLayout::eUndefined
	);

	vk::Image depth;
	VmaAllocation depth_alloction = {};
	VmaAllocationCreateInfo depth_alloc_info = {};
	depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	check_error(vmaCreateImage(d_allocator, (VkImageCreateInfo*)& image_ci,
		&depth_alloc_info, (VkImage*)& depth, &depth_alloction, nullptr));

	d_depth.image = depth;
	d_depth.meta = depth_alloction;

	d_depth.view = d_logical_device.createImageView(
		vk::ImageViewCreateInfo(
		vk::ImageViewCreateFlags(),
		d_depth.image,
		vk::ImageViewType::e2D,
		d_surfaceDepthFormat,
		vk::ComponentMapping(),
		vk::ImageSubresourceRange(
		vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
		0,
		1,
		0,
		1
	)
	)
	);

	auto colorImagesInSwapchain = d_logical_device.getSwapchainImagesKHR(d_swapchain);

	assert(colorImagesInSwapchain.size() == d_swapchainFrameBuffers.size());

	for (size_t i = 0; i < colorImagesInSwapchain.size(); i++)
	{
		// Color
		d_swapchainFrameBuffers[i].views[COLOR] =
			d_logical_device.createImageView(
			vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(),
			colorImagesInSwapchain[i],
			vk::ImageViewType::e2D,
			d_surfaceColorFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
			vk::ImageAspectFlagBits::eColor,
			0,
			1,
			0,
			1
			)
			)
			);

		// Depth
		d_swapchainFrameBuffers[i].views[DEPTH] = d_depth.view;

		// frame buffer
		d_swapchainFrameBuffers[i].frameBuffer = d_logical_device.createFramebuffer(
			vk::FramebufferCreateInfo(
			vk::FramebufferCreateFlags(),
			d_renderPass,
			static_cast<uint32_t>(d_swapchainFrameBuffers[i].views.size()),
			d_swapchainFrameBuffers[i].views.data(),
			d_surfaceSize.width, d_surfaceSize.height,
			1
		)
		);
	}
}

void Context::setupDrawCommandsAndSynchronization()
{
	// create draw commadns
	d_commandBuffers = d_logical_device.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo(
		d_queues[graphics].cmdPools,
		vk::CommandBufferLevel::ePrimary,
		d_settings.backbuffer_count
	)
	);

	// Semaphore used to ensures that image presentation is complete before starting to submit again
	d_presentCompleteSemaphores.resize(d_settings.backbuffer_count);

	// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
	d_renderCompleteSemaphores.resize(d_settings.backbuffer_count);

	// Fence for command buffer completion
	d_waitFences.resize(d_settings.backbuffer_count);

	for (size_t i = 0; i < d_waitFences.size(); i++)
	{

		d_presentCompleteSemaphores[i] = d_logical_device.createSemaphore(vk::SemaphoreCreateInfo());

		d_renderCompleteSemaphores[i] = d_logical_device.createSemaphore(vk::SemaphoreCreateInfo());

		d_waitFences[i] = d_logical_device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}
}

void Context::setupDescriptorPool()
{
	//Descriptor Pool
	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes =
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
		/*	vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),*/

			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
			//vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),

			//vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
	};

	d_descriptorPool = d_logical_device.createDescriptorPool(
		vk::DescriptorPoolCreateInfo(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		/*vk::DescriptorPoolCreateFlags(),*/
		static_cast<uint32_t>(1000 * descriptorPoolSizes.size()),
		static_cast<uint32_t>(descriptorPoolSizes.size()),
		descriptorPoolSizes.data()
	)
	);
}

void Context::destroyRenderpass()
{
	d_logical_device.destroyRenderPass(d_renderPass);
}

void Context::destroyFrameBuffers()
{
	for (auto& elem : d_swapchainFrameBuffers)
	{
		d_logical_device.destroyFramebuffer(elem.frameBuffer);
		d_logical_device.destroyImageView(elem.views[COLOR]);
	}
	d_logical_device.destroyImageView(d_depth.view);
	vmaDestroyImage(d_allocator, d_depth.image, d_depth.meta);
}

void Context::destroyDrawCommandsAndSynchronization()
{
	d_logical_device.freeCommandBuffers(d_queues[graphics].cmdPools, d_commandBuffers);

	for (int i = 0; i < QueueType::QueueTypeSize; ++i)
	{
		d_logical_device.resetCommandPool(d_queues[i].cmdPools, vk::CommandPoolResetFlagBits::eReleaseResources);
	}

	for (size_t i = 0; i < d_waitFences.size(); i++)
	{
		d_logical_device.destroySemaphore(d_presentCompleteSemaphores[i]);
		d_logical_device.destroySemaphore(d_renderCompleteSemaphores[i]);
		d_logical_device.destroyFence(d_waitFences[i]);
	}
}

bool Context::hasStencilComponent(vk::Format format)
{
	return
		format == vk::Format::eD16UnormS8Uint ||
		format == vk::Format::eD24UnormS8Uint ||
		format == vk::Format::eD32SfloatS8Uint;

}

void Context::setupMemoryAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = d_physcial_device;
	allocatorInfo.device = d_logical_device;
	vmaCreateAllocator(&allocatorInfo, &d_allocator);
}

} // end namespace vkapi