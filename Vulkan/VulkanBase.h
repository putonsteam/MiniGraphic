#pragma once
#include "VulkanHeader.h"

class CVulkanBase
{
public:
	struct RenderBuffer
	{
		VkImage image;
		VkImageView imageView;
		//  A framebuffer provides the attachments that a render pass needs while rendering
		// VkRenderpass only describe \
		// The framebuffer essentially associates the actual attachments to the renderpass.
		VkFramebuffer frameBuffer;	
		VkCommandBuffer commandBuffer;
		VkFence fence;
	};

	struct DepthBuffer
	{
		VkImage image;
		VkImageView imageView;
		VkFormat format;
	};

public:
	virtual bool Initialize(HINSTANCE hInstance, HWND hWnd);
	virtual void Run() = 0;
	virtual void Release();

protected:
	bool BeginCommandBuffer(VkCommandBuffer cmd);
	bool EndCommandBuffer(VkCommandBuffer cmd);
	bool FlushCommandBuffer(VkCommandBuffer cmd);
	bool AllocMemory(const VkMemoryRequirements & _memory_requirement, VkFlags required_mask,
		VkDeviceMemory* _device_memory);
	void SetImageLayout(VkCommandBuffer _cmd, VkImageAspectFlags  _aspectMask, VkImage _image,
		VkImageLayout _old_layout, VkImageLayout _new_layout, VkAccessFlagBits _srcAccessFlags);

private:
	bool InitializeInstanceLayerAndExt();
	bool InitializeInstance();
	bool InitializeSurface();
	bool InitializeGpu();
	bool InitializeDeviceLayerAndExt();
	bool InitializeQueueFamilyIndex();
	bool InitializeDeviceAndQueue();
	bool InitializeSwapChain();
	bool InitializeCommandBuffer();
	bool InitializeRenderBuffer();
	bool InitializeDepthBuffer();
	bool InitializeRenderPass();
	bool InitializeFrameBuffer();
	bool InitializeDebugCallb();
	bool InitializeFrameSync();

	virtual bool InilializeDescriptorPool();
	virtual bool InilializeDescriptorSetLayout();
	virtual bool InilializePipelineLayout();

protected:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	VkInstance m_VulkanInstance;
	VkSurfaceKHR m_VulkanSurface;    // surface refer render target
	VkExtent2D m_SurfaceExtent;
	VkFormat m_SurfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkPhysicalDevice m_VulkanGpu;
	VkDevice m_VulkanDevice;
	uint32_t m_QueueFamilyIndex = -1;
	VkQueue m_VulkanQueue;
	VkSwapchainKHR m_VulkanSwapchain;
	VkCommandPool m_VulkanCommandPool;
	VkCommandBuffer m_VulkanCommandBuffer;
	uint32_t m_uSwapchainImageCount = 0;
	RenderBuffer *m_pRenderBufferArray;
	DepthBuffer m_DepthBuffer;
	VkRenderPass m_VulkanRenderPass;   // describe the framebuffer
	VkDescriptorPool m_DescriptorPool; // allocate Descriptor Set
	VkDescriptorSetLayout m_DescriptorSetLayout;	// the layout for the Descriptor Set 
	VkPipelineLayout m_PipelineLayout;		// the layout for pipeline
	VkSemaphore m_AcquireImageSemaphore;
	VkSemaphore m_RenderingDoneSemaphore;
	uint32_t m_CurrentSwapBuffer = 0;

private:
	std::vector<char*>				m_InstanceLayerNames;
	std::vector<char*>				m_InstanceExtensionNames;
	std::vector<char*>				m_DeviceLayerNames;
	std::vector<char*>				m_DeviceExtensionNames;

	VkDebugReportCallbackEXT m_DebugCallback;
};

