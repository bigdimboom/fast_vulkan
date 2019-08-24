#pragma once
#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <chrono>

#include "../window/vk_window.h"
#include "../camera/free_camera.h"

#include <memory>

namespace renderer
{

class Renderer
{
public:
	Renderer(window::VKWindow& window, std::shared_ptr<camera::FreeCamera> camera = nullptr);

	~Renderer();

	// Render onto the render target
	void render();

	// Resize the window and internal data structures
	void resize(unsigned width, unsigned height);

protected:

	// Initialize your Graphics API
	void initializeAPI(window::VKWindow& window);

	// Destroy any Graphics API data structures used in this example
	void destroyAPI();

	// Initialize any resources such as VBOs, IBOs, used in this example
	void initializeResources();

	// Destroy any resources used in this example
	void destroyResources();

	// Create graphics API specific data structures to send commands to the GPU
	void createCommands();

	// Set up commands used when rendering frame by this app
	void setupCommands();

	// Destroy all commands
	void destroyCommands();

	// Set up the FrameBuffer
	void initFrameBuffer();

	void destroyFrameBuffer();

	// Set up the RenderPass
	void createRenderPass();

	void createSynchronization();

	// Set up the swapchain
	void setupSwapchain(unsigned width, unsigned height);

	struct Vertex
	{
		float position[3];
		float color[3];
	};

	Vertex mVertexBufferData[3] =
	{
	  { { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
	  { { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
	  { { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};

	uint32_t mIndexBufferData[3] = { 0, 1, 2 };

	std::chrono::time_point<std::chrono::steady_clock> tStart, tEnd;
	float mElapsedTime = 0.0f;

	// Uniform data
	struct {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	} uboVS;

	// Initialization
	vk::Instance mInstance;
	vk::DispatchLoaderDynamic mDldy;
	vk::DebugReportCallbackEXT mDebug;
	vk::PhysicalDevice mPhysicalDevice;
	vk::Device mDevice;

	vk::SwapchainKHR mSwapchain;
	vk::SurfaceKHR mSurface;

	float mQueuePriority;
	vk::Queue mQueue;
	uint32_t mQueueFamilyIndex;

	vk::CommandPool mCommandPool;
	std::vector<vk::CommandBuffer> mCommandBuffers;
	uint32_t mCurrentBuffer;

	vk::Extent2D mSurfaceSize;
	vk::Rect2D mRenderArea;
	vk::Viewport mViewport;

	// Resources
	vk::Format mSurfaceColorFormat;
	vk::ColorSpaceKHR mSurfaceColorSpace;
	vk::Format mSurfaceDepthFormat;
	vk::Image mDepthImage;
	vk::DeviceMemory mDepthImageMemory;

	vk::DescriptorPool mDescriptorPool;
	std::vector<vk::DescriptorSetLayout> mDescriptorSetLayouts;
	std::vector<vk::DescriptorSet> mDescriptorSets;

	vk::ShaderModule mVertModule;
	vk::ShaderModule mFragModule;

	vk::RenderPass mRenderPass;

	vk::Buffer mVertexBuffer;
	vk::Buffer mIndexBuffer;

	vk::PipelineCache mPipelineCache;
	vk::Pipeline mPipeline;
	vk::PipelineLayout mPipelineLayout;

	// Sync
	vk::Semaphore mPresentCompleteSemaphore;
	vk::Semaphore mRenderCompleteSemaphore;
	std::vector<vk::Fence> mWaitFences;

	// Swpachain
	struct SwapChainBuffer {
		vk::Image image;
		std::array<vk::ImageView, 2> views;
		vk::Framebuffer frameBuffer;
	};

	std::vector<SwapChainBuffer> mSwapchainBuffers;

	// Vertex buffer and attributes
	struct {
		vk::DeviceMemory memory;														// Handle to the device memory for this buffer
		vk::Buffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
		vk::PipelineVertexInputStateCreateInfo inputState;
		vk::VertexInputBindingDescription inputBinding;
		std::vector<vk::VertexInputAttributeDescription> inputAttributes;
	} mVertices;

	// Index buffer
	struct
	{
		vk::DeviceMemory memory;
		vk::Buffer buffer;
		uint32_t count;
	} mIndices;

	// Uniform block object
	struct {
		vk::DeviceMemory memory;
		vk::Buffer buffer;
		vk::DescriptorBufferInfo descriptor;
	}  mUniformDataVS;

	std::shared_ptr<camera::FreeCamera> mCamera;
};


} //end namespace demo