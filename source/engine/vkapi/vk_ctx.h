#pragma once
#include "../window/vk_window.h"
#include <array>
#include "data_type.h"
#include <functional>
#include <map>

namespace vkapi
{

struct CtxSettings
{
	uint32_t gpuid = 0;
	bool debug = true;
	bool vsync = true;
	uint32_t backbuffer_count = 3;
	std::string appname = "vkapi";
	std::string enginename = "vkapi_engine";
	bool standalone_compute_queue = false;
	bool standalone_transfer_queue = false;
};

class Context
{
public:
	Context(window::VKWindow& vkWindow, const CtxSettings& settings);
	~Context();

	Context(const Context&) = delete;
	Context(Context&&) = delete;
	void operator=(const Context&) = delete;
	void operator=(Context&&) = delete;

	vk::Instance vkInstance() const;
	vk::PhysicalDevice vkPhysicalDevice() const;
	vk::Device vkDevice() const;
	vk::SurfaceKHR vkSurface() const;
	vk::SwapchainKHR vkSwapchain() const;
	vk::DescriptorPool vkDescriptorPool() const;
	vk::RenderPass defaultRenderPass() const;
	vk::CommandBuffer commandBuffer() const;
	vk::Viewport viewport() const;
	vk::Rect2D renderArea() const;

	VmaAllocator memAllocator() const;

	vk::Queue vkQueue(vk::QueueFlagBits flag = vk::QueueFlagBits::eGraphics) const;
	vk::CommandPool vkCmdPool(vk::QueueFlagBits flag = vk::QueueFlagBits::eGraphics) const;
	uint32_t familyQueueIndex(vk::QueueFlagBits flag = vk::QueueFlagBits::eGraphics) const;

	uint32_t nSwapchainFrameBuffers() const;

	static void check_error(vk::Result result);
	static void check_error(VkResult result);

	void resize();

	void setClearValue(float r, float g, float b, float a = 1.0f);
	void setClearValues(vk::ClearColorValue color, vk::ClearDepthStencilValue depth = { 1.0f, 0u });
	void frameBegin();
	void beginDefaultRenderPass();
	void flushStaticDraws();
	void endDefaultRenderPass();
	void frameEnd();
	void framePresent();

	vk::CommandBuffer beginSingleTimeCommands(bool begin);
	void flushSingleTimeCommands(vk::CommandBuffer& cmd, bool end);

	void addStaticDraw(std::function<void(vk::CommandBuffer, vk::RenderPass)> func, const char* key, vk::RenderPass renderpass = nullptr);
	void removeStaticDraw(const char* key);
	void removeAllStaticDraws();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	VkShaderModule createShaderModule(const std::string& filename);

	void copy(vk::Buffer dst, vk::Buffer src, const vk::BufferCopy& region);
	void copy(vk::Image  dst, vk::Buffer src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
	void copy(vk::Buffer dst, vk::Image  src, const vk::BufferImageCopy& region, vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal);
	void upload(BufferObject& dst_hostVisable, void* src_host, size_t size_bytes);

	std::shared_ptr<BufferObject> createSharedBufferObject(const vk::BufferCreateInfo& bufferinfo, const VmaAllocationCreateInfo& alloc_info);

	std::shared_ptr<ImageObject> createSharedImageObject(const vk::ImageCreateInfo& image_info, const VmaAllocationCreateInfo& alloc_info);

	std::shared_ptr<ImageObject> createTextureImage2D(int width, int height, vk::Format format, void* hostdata_ptr);

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

	template<class T>
	std::shared_ptr<BufferObject> createVertexBufferObject(const std::vector<T>& data);

	std::shared_ptr<BufferObject> createUniformBufferObject(uint64_t size);

protected:
	// basic context
	void setupVulkanInstance();
	void setupPhsicalDevice();
	void setupLogicalDevice();
	void setupMemoryAllocator();
	void setupSurface();
	// swapchain related context
	void setupSwapchain();
	void checkSurfaceFormat();
	void setupRenderPass();
	void setupFrameBuffer();
	void setupDrawCommandsAndSynchronization();
	// resource
	void setupDescriptorPool();

	// destroy methods
	void destroyRenderpass();
	void destroyFrameBuffers();
	void destroyDrawCommandsAndSynchronization();

protected:

	window::VKWindow& d_window;
	CtxSettings d_settings;

	// init system
	vk::Instance d_instance;
	vk::PhysicalDevice d_physcial_device;
	vk::DispatchLoaderDynamic d_Dldy;
	vk::DebugReportCallbackEXT d_debugReport;

	// logical device
	vk::Device d_logical_device;

	// allocator
	VmaAllocator d_allocator;

	// queues
	enum QueueType
	{
		graphics = 0,
		compute,
		transfer,
		QueueTypeSize
	};

	struct Queue
	{
		uint32_t familyQueueIndex;
		vk::Queue vkQueue;
		vk::CommandPool cmdPools;
	};

	std::array<Queue, QueueType::QueueTypeSize> d_queues;

	// swapchain
	vk::SurfaceKHR d_surface;
	vk::SwapchainKHR d_swapchain;
	vk::Rect2D d_renderArea;
	vk::Viewport d_viewport;
	vk::Extent2D d_surfaceSize;
	vk::Format d_surfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
	vk::ColorSpaceKHR d_surfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	vk::Format d_surfaceDepthFormat = vk::Format::eD24UnormS8Uint;

	// render pass
	vk::RenderPass d_renderPass;
	std::array<vk::ClearValue, 2> d_clearValues;

	// frame buffer
	enum FrameType
	{
		COLOR = 0,
		DEPTH,
		FrameTypeSize
	};

	struct SwapChainFrameBuffer
	{
		std::array<vk::ImageView, FrameTypeSize> views;
		vk::Framebuffer frameBuffer;
	};

	struct DepthImageMem
	{
		vk::Image image;
		VmaAllocation meta;
		vk::ImageView view;
	}d_depth;

	std::vector<SwapChainFrameBuffer> d_swapchainFrameBuffers;

	uint32_t d_frameIndex = 0;

	// sync objects
	std::vector<vk::Semaphore> d_presentCompleteSemaphores;
	std::vector<vk::Semaphore> d_renderCompleteSemaphores;
	std::vector<vk::Fence> d_waitFences;

	// comman buffer
	std::vector<vk::CommandBuffer> d_commandBuffers;

	// static draw secondary command buffer
	std::map<std::string, std::vector<vk::CommandBuffer>> d_staticDraws;

	// descriptor
	vk::DescriptorPool d_descriptorPool;

	// pipeline cache
	vk::PipelineCache d_pipelineCache = nullptr;

private:
	bool hasStencilComponent(vk::Format format);

};


template<class T>
inline std::shared_ptr<BufferObject> Context::createVertexBufferObject(const std::vector<T>& data)
{
	vk::BufferCreateInfo stagingBufferInfo = {};
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingBufferInfo.size = data.size() * sizeof(T);
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	auto stagebuffer = createSharedBufferObject(stagingBufferInfo, stagingAllocInfo);
	upload(*stagebuffer, (void*)data.data(), data.size() * sizeof(T));

	vk::BufferCreateInfo vboInfo = {};
	VmaAllocationCreateInfo vboAllocInfo = {};
	vboInfo.size = data.size() * sizeof(T);
	vboInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
	vboAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	auto vbo = createSharedBufferObject(vboInfo, vboAllocInfo);

vk:VkBufferCopy region = {};
	region.size = data.size() * sizeof(T);
	copy(vbo->buffer, stagebuffer->buffer, region);

	return vbo;
}


} // end namepsace vkapi