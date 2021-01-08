#include "VulkanBase.h"

// show debug info, contain error to debug
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugReportFlagsEXT                       flags,
	VkDebugReportObjectTypeEXT                  objectType,
	uint64_t                                    object,
	size_t                                      location,
	int32_t                                     messageCode,
	const char*                                 pLayerPrefix,
	const char*                                 pMessage,
	void*                                       pUserData)
{
	std::ostringstream message;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		message << "ERROR: ";
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		message << "WARNING: ";
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		message << "PERFORMANCE WARNING: ";
	}
	else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		message << "INFO: ";
	}
	else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		message << "DEBUG: ";
	}

	message << "[" << pLayerPrefix << "] Code " << messageCode << " : " << pMessage;
	OutputDebugString(message.str().c_str());
	OutputDebugString("\n");

	assert((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) == 0);

	return false;
}

bool CVulkanBase::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;

	if (!InitializeInstanceLayerAndExt())
	{
		return false;
	}
	if (!InitializeInstance())
	{
		return false;
	}
	if (!InitializeDebugCallb())
	{
		return false;
	}
	if (!InitializeSurface())
	{
		return false;
	}
	if (!InitializeGpu())
	{
		return false;
	}
	if (!InitializeDeviceLayerAndExt())
	{
		return false;
	}
	if (!InitializeQueueFamilyIndex())
	{
		return false;
	}
	if (!InitializeDeviceAndQueue())
	{
		return false;
	}
	if (!InitializeSwapChain())
	{
		return false;
	}
	if (!InitializeCommandBuffer())
	{
		return false;
	}
	if (!InilializeDescriptorPool())
	{
		return false;
	}
	if (!InilializeDescriptorSetLayout())
	{
		return false;
	}
	if (!InilializePipelineLayout())
	{
		return false;
	}
	if (BeginCommandBuffer(m_VulkanCommandBuffer))
	{
		if (!InitializeRenderBuffer())
		{
			return false;
		}
		if (!InitializeDepthBuffer())
		{
			return false;
		}
		EndCommandBuffer(m_VulkanCommandBuffer);
		FlushCommandBuffer(m_VulkanCommandBuffer);
	}
	else
	{
		return false;
	}
	if (!InitializeRenderPass())
	{
		return false;
	}
	if (!InitializeFrameBuffer())
	{
		return false;
	}
	if (!InitializeFrameSync())
	{
		return false;
	}
}

bool CVulkanBase::InitializeFrameSync()
{
	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VK_RETURN_IF_FAILED(vkCreateSemaphore(m_VulkanDevice, 
		&semaphoreInfo, nullptr, &m_AcquireImageSemaphore));
	VK_RETURN_IF_FAILED(vkCreateSemaphore(m_VulkanDevice, 
		&semaphoreInfo, nullptr, &m_RenderingDoneSemaphore));

	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	for (int i = 0; i < m_uSwapchainImageCount; ++i) 
	{
		VK_RETURN_IF_FAILED(vkCreateFence(m_VulkanDevice, &fenceCreateInfo,
			nullptr, &m_pRenderBufferArray[i].fence));
	}

	return true;
}

bool CVulkanBase::BeginCommandBuffer(VkCommandBuffer cmd)
{
	VkCommandBufferBeginInfo cmdBeginInfo = 
	{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_RETURN_IF_FAILED(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	return true;
}

// because the opposite of BeginCommandBuffer
bool CVulkanBase::EndCommandBuffer(VkCommandBuffer cmd)
{
	VK_RETURN_IF_FAILED(vkEndCommandBuffer(cmd));

	return true;
}

bool CVulkanBase::FlushCommandBuffer(VkCommandBuffer cmd)
{
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VK_RETURN_IF_FAILED(vkQueueSubmit(m_VulkanQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_RETURN_IF_FAILED(vkQueueWaitIdle(m_VulkanQueue));

	return true;
}


// show debug info
bool CVulkanBase::InitializeDebugCallb()
{
	PFN_vkCreateDebugReportCallbackEXT	create_DebugCallback =
		(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
			m_VulkanInstance, "vkCreateDebugReportCallbackEXT");
	assert(create_DebugCallback);

	VkDebugReportCallbackCreateInfoEXT Info =
	{ VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	Info.pNext = nullptr;
	Info.pUserData = nullptr;
	Info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT
		| VK_DEBUG_REPORT_WARNING_BIT_EXT
		| VK_DEBUG_REPORT_ERROR_BIT_EXT
		| VK_DEBUG_REPORT_DEBUG_BIT_EXT
		| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	Info.pfnCallback = &DebugCallback;

	VK_RETURN_IF_FAILED(create_DebugCallback(m_VulkanInstance, &Info,
		nullptr, &m_DebugCallback));

	return true;
}

bool CVulkanBase::InilializePipelineLayout()
{
	VkPipelineLayoutCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	info.pSetLayouts = &m_DescriptorSetLayout;
	info.setLayoutCount = 1;
	VK_RETURN_IF_FAILED(vkCreatePipelineLayout(m_VulkanDevice, &info,
		nullptr, &m_PipelineLayout));

	return true;
}

bool CVulkanBase::InilializeDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding bind[2] = {};
	bind[0].descriptorCount = 1;
	// correspond to a resource of the same binding in the shader stages
	bind[0].binding = 0;	// index
	bind[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bind[0].pImmutableSamplers = nullptr;
	bind[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bind[1].descriptorCount = 1;
	bind[1].binding = 1;	// index
	bind[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bind[1].pImmutableSamplers = nullptr;
	bind[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo info = 
	{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.bindingCount = 2;
	info.pBindings = bind;
	info.pNext = nullptr;

	VK_RETURN_IF_FAILED(vkCreateDescriptorSetLayout(m_VulkanDevice, &info,
		nullptr, &m_DescriptorSetLayout));

	return true;
}

bool CVulkanBase::InilializeDescriptorPool()
{
	VkDescriptorPoolSize poolSize[2];
	poolSize[0].descriptorCount = 1;
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[1].descriptorCount = 1;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.maxSets = 1;
	info.poolSizeCount = 2;
	info.pPoolSizes = poolSize;
	VK_RETURN_IF_FAILED(vkCreateDescriptorPool(m_VulkanDevice, &info,
		nullptr, &m_DescriptorPool));

	return true;
}

// translate the image state
// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL is the best layout for the GPU rendering
// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR is about to be presented to the display
void CVulkanBase::SetImageLayout(VkCommandBuffer cmd, VkImageAspectFlags  aspectMask,
	VkImage image, VkImageLayout oldLayout,
	VkImageLayout newLayout, VkAccessFlagBits srcAccessFlags)

{
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcAccessMask = srcAccessFlags;
	imageMemoryBarrier.dstAccessMask = 0;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;

	if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		/* Make sure anything that was copying from this image has completed */
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		/* Make sure any Copy or CPU writes to image are flushed */
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
			| VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	}

	VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmd, srcStages, destStages, 0, 0, nullptr, 0, nullptr, 1,
		&imageMemoryBarrier);
}

bool CVulkanBase::InitializeFrameBuffer()
{
	for (int i = 0; i != m_uSwapchainImageCount; ++i)
	{
		VkImageView attach[2] =
		{
			m_pRenderBufferArray[i].imageView,
			m_DepthBuffer.imageView,
		};
		VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		info.attachmentCount = 2;
		info.height = m_SurfaceExtent.height;
		info.width = m_SurfaceExtent.width;
		info.renderPass = m_VulkanRenderPass;
		info.layers = 1;
		info.pAttachments = attach;

		VK_RETURN_IF_FAILED(vkCreateFramebuffer(m_VulkanDevice, &info,
			nullptr, &m_pRenderBufferArray[i].frameBuffer));
	}
	return true;
}

bool CVulkanBase::InitializeRenderPass()
{
	VkAttachmentDescription attach[2] = {};
	attach[0].format = m_SurfaceFormat;
	attach[0].samples = VK_SAMPLE_COUNT_1_BIT;
	//  indicates that you want the buffer to be cleared at the start of the render pass instance.
	attach[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// means that you want to leave the rendering result in this buffer, so it can be presented to the display.
	attach[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attach[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attach[1].format = m_DepthBuffer.format;
	attach[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attach[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// means that you don't need the contents of the buffer
	attach[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attach[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	// above attachment index
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	// above attachment index
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attach;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = nullptr;

	VK_RETURN_IF_FAILED(vkCreateRenderPass(m_VulkanDevice, &renderPassInfo,
		nullptr, &m_VulkanRenderPass));

	return true;
}

bool CVulkanBase::InitializeRenderBuffer()
{
	std::unique_ptr<VkImage[]> renderImage(new VkImage[m_uSwapchainImageCount]);
	//std::unique_ptr<VkImageView[]> renderViewImage(new VkImage[m_uSwapchainImageCount]);
	// get image
	VK_RETURN_IF_FAILED(vkGetSwapchainImagesKHR(m_VulkanDevice, m_VulkanSwapchain,
		&m_uSwapchainImageCount, renderImage.get()));

	// create image views
	for (int i = 0; i != m_uSwapchainImageCount; ++i)
	{
		VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		info.components.r = VK_COMPONENT_SWIZZLE_R;
		info.components.g = VK_COMPONENT_SWIZZLE_G;
		info.components.b = VK_COMPONENT_SWIZZLE_B;
		info.components.a = VK_COMPONENT_SWIZZLE_A;
		info.format = m_SurfaceFormat;
		info.image = renderImage[i];
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VK_RETURN_IF_FAILED(vkCreateImageView(m_VulkanDevice, &info, nullptr,
			&m_pRenderBufferArray[i].imageView));
		m_pRenderBufferArray[i].image = renderImage[i];

		// translate state
		SetImageLayout(
			m_VulkanCommandBuffer,
			VK_IMAGE_ASPECT_COLOR_BIT,
			m_pRenderBufferArray[i].image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			(VkAccessFlagBits)0);
	}
	return true;
}

bool CVulkanBase::InitializeDepthBuffer()
{
	m_DepthBuffer.format = VK_FORMAT_D16_UNORM;

	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.format = m_DepthBuffer.format;
	imageInfo.extent.depth = 1;
	imageInfo.extent.width = m_SurfaceExtent.width;
	imageInfo.extent.height = m_SurfaceExtent.height;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VK_RETURN_IF_FAILED(vkCreateImage(m_VulkanDevice,
		&imageInfo, nullptr, &m_DepthBuffer.image));

	VkMemoryRequirements imageMemoryReq;
	vkGetImageMemoryRequirements(m_VulkanDevice, m_DepthBuffer.image, &imageMemoryReq);
	// allocate memory
	VkDeviceMemory imageMemory;
	if (!AllocMemory(imageMemoryReq, 0, &imageMemory))
	{
		return false;
	}
	// bind memory
	VK_RETURN_IF_FAILED(vkBindImageMemory(m_VulkanDevice, m_DepthBuffer.image, imageMemory, 0));

	// translate state 
	SetImageLayout(
		m_VulkanCommandBuffer,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_DepthBuffer.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	
		(VkAccessFlagBits)0);

	// create image view
	VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.format = m_DepthBuffer.format;
	viewInfo.image = m_DepthBuffer.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = 1;

	VK_RETURN_IF_FAILED(vkCreateImageView(m_VulkanDevice, &viewInfo,
		nullptr, &m_DepthBuffer.imageView));

	return true;
}

bool CVulkanBase::AllocMemory(
	const VkMemoryRequirements & _memory_requirement,
	VkFlags required_mask,
	VkDeviceMemory* _device_memory)
{
	//Get the momery properties
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_VulkanGpu, &memoryProperties);

	VkMemoryAllocateInfo _memory_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	_memory_alloc_info.allocationSize = _memory_requirement.size;

	uint32_t _type_bits = _memory_requirement.memoryTypeBits;

	for (auto i = 0u; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((_type_bits & 1))
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & required_mask)
				== required_mask)
			{
				_memory_alloc_info.memoryTypeIndex = i;
				break;
			}
		}

		_type_bits >>= 1;
	}

	assert(_device_memory);
	//alloc memory
	VK_RETURN_IF_FAILED(vkAllocateMemory(m_VulkanDevice, &_memory_alloc_info,
		nullptr, _device_memory));

	return true;
}

bool CVulkanBase::InitializeCommandBuffer()
{
	// create command buffer pool
	VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.queueFamilyIndex = m_QueueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_RETURN_IF_FAILED(vkCreateCommandPool(
		m_VulkanDevice, &poolInfo, nullptr, &m_VulkanCommandPool));

	VkCommandBufferAllocateInfo allocateInfo =
	{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = m_VulkanCommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	m_pRenderBufferArray = new RenderBuffer[m_uSwapchainImageCount];
	for (int i = 0; i < m_uSwapchainImageCount; ++i) 
	{
		VK_RETURN_IF_FAILED(vkAllocateCommandBuffers(m_VulkanDevice, &allocateInfo, 
			&m_pRenderBufferArray[i].commandBuffer));
	}

	VK_RETURN_IF_FAILED(vkAllocateCommandBuffers(
		m_VulkanDevice, &allocateInfo, &m_VulkanCommandBuffer));
	return true;
}

bool CVulkanBase::InitializeSwapChain()
{
	// get capability
	VkSurfaceCapabilitiesKHR capability;
	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		m_VulkanGpu, m_VulkanSurface, &capability));

	// get extent 2d
	m_SurfaceExtent = capability.currentExtent;

	// get min image count 
	uint32_t minImageCount = min(
		capability.minImageCount + 1, capability.maxImageCount);

	// get format
	uint32_t formatCount = 0;
	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(
		m_VulkanGpu, m_VulkanSurface, &formatCount, nullptr));
	if (formatCount > 0)
	{
		std::unique_ptr<VkSurfaceFormatKHR[]> format(
			new VkSurfaceFormatKHR[formatCount]);
		VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(
			m_VulkanGpu, m_VulkanSurface, &formatCount, format.get()));
		if (format[0].format != VK_FORMAT_UNDEFINED)
		{
			m_SurfaceFormat = format[0].format;
		}
	}

	// get present mode
	uint32_t presentModeCount = 0;
	VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(
		m_VulkanGpu, m_VulkanSurface, &presentModeCount, nullptr));
	VkPresentModeKHR nowPresent = VK_PRESENT_MODE_FIFO_KHR;
	if (presentModeCount > 0)
	{
		std::unique_ptr<VkPresentModeKHR[]> presentMode(
			new VkPresentModeKHR[presentModeCount]);
		VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(
			m_VulkanGpu, m_VulkanSurface, &presentModeCount, presentMode.get()));
		// 1. Mailbox , less latency , less tearing
		// 2. Immediate, fast but tearing
		// 3. FIFO, all support
		for (int i = 0; i != presentModeCount; ++i)
		{
			if (presentMode[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				nowPresent = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			else if (presentMode[i] == VK_PRESENT_MODE_FIFO_KHR)
			{
				nowPresent = VK_PRESENT_MODE_FIFO_KHR;
			}
			else if (presentMode[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				nowPresent = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// get transform
	VkSurfaceTransformFlagBitsKHR transform = capability.currentTransform;
	if (transform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	// prepare create swapchain info
	VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	info.clipped = VK_TRUE;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.flags = 0;
	info.imageArrayLayers = 1;
	info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	info.imageExtent = m_SurfaceExtent;
	info.imageFormat = m_SurfaceFormat;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.minImageCount = minImageCount;
	info.oldSwapchain = NULL;
	info.pNext = nullptr;
	info.pQueueFamilyIndices = nullptr;
	info.presentMode = nowPresent;
	info.preTransform = transform;
	info.queueFamilyIndexCount = 0;
	info.surface = m_VulkanSurface;

	VK_RETURN_IF_FAILED(vkCreateSwapchainKHR(
		m_VulkanDevice, &info, nullptr, &m_VulkanSwapchain));

	VK_RETURN_IF_FAILED(vkGetSwapchainImagesKHR(
		m_VulkanDevice, m_VulkanSwapchain, &m_uSwapchainImageCount, nullptr));
	assert(m_uSwapchainImageCount > 0);

	return true;
}

bool CVulkanBase::InitializeDeviceAndQueue()
{
	// prepare queue create info
	float fPriority[] = { 1.0f };
	VkDeviceQueueCreateInfo queueInfo =
	{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.pQueuePriorities = fPriority;
	queueInfo.queueFamilyIndex = m_QueueFamilyIndex;
	queueInfo.queueCount = 1;

	// prepare device create info
	VkDeviceCreateInfo deviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceInfo.enabledLayerCount = m_DeviceLayerNames.size();
	deviceInfo.ppEnabledLayerNames = m_DeviceLayerNames.data();
	deviceInfo.enabledExtensionCount = m_DeviceExtensionNames.size();
	deviceInfo.ppEnabledExtensionNames = m_DeviceExtensionNames.data();
	deviceInfo.pQueueCreateInfos = &queueInfo;
	//It is a must to create 1 queue for graphic
	deviceInfo.queueCreateInfoCount = 1;

	VK_RETURN_IF_FAILED(vkCreateDevice(m_VulkanGpu, &deviceInfo, nullptr, &m_VulkanDevice));
	vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilyIndex, 0, &m_VulkanQueue);
	return true;
}

bool CVulkanBase::InitializeQueueFamilyIndex()
{
	uint32_t familyProCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanGpu, &familyProCount, nullptr);
	if (familyProCount > 0)
	{
		std::unique_ptr<VkBool32[]> supportPresent(new VkBool32[familyProCount]);
		for (int i = 0; i != familyProCount; ++i)
		{
			VK_RETURN_IF_FAILED(vkGetPhysicalDeviceSurfaceSupportKHR(
				m_VulkanGpu, i, m_VulkanSurface, &supportPresent[i]));
		}
		std::unique_ptr<VkQueueFamilyProperties[]> familyPro(
			new VkQueueFamilyProperties[familyProCount]);
		vkGetPhysicalDeviceQueueFamilyProperties(
			m_VulkanGpu, &familyProCount, familyPro.get());
		for (int i = 0; i != familyProCount; ++i)
		{
			if (familyPro[i].queueFlags & VK_QUEUE_GRAPHICS_BIT
				&& supportPresent[i] == VK_TRUE)
			{
				// tree structure, device -- family -- queue
				m_QueueFamilyIndex = i;
				break;
			}
		}
		if (m_QueueFamilyIndex == -1)
		{
			assert(0 && "failed to find propers queue family index!");
			return false;
		}
		return true;
	}
	return false;
}

bool CVulkanBase::InitializeDeviceLayerAndExt()
{
	//enumerate layer
	uint32_t _count = 0;
	VK_RETURN_IF_FAILED(vkEnumerateDeviceLayerProperties(m_VulkanGpu, &_count, nullptr));
	std::unique_ptr<VkLayerProperties[]> _vkLayerProperties(new VkLayerProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateDeviceLayerProperties(
		m_VulkanGpu, &_count, _vkLayerProperties.get()));

	for (int i = 0; i < _count; ++i)
	{
		OutputDebugString(_vkLayerProperties[i].layerName);
		OutputDebugString("     ");
		OutputDebugString(_vkLayerProperties[i].description);
		OutputDebugString("\n");
		m_DeviceLayerNames.push_back(_strdup(_vkLayerProperties[i].layerName));
	}

	//enumerate extension
	VK_RETURN_IF_FAILED(vkEnumerateDeviceExtensionProperties(m_VulkanGpu, nullptr, &_count, nullptr));
	std::unique_ptr<VkExtensionProperties[]> _vkExtensionProperties(
		new VkExtensionProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateDeviceExtensionProperties(
		m_VulkanGpu, nullptr, &_count, _vkExtensionProperties.get()));

	for (auto i = 0u; i < _count; ++i)
	{
		OutputDebugString(_vkExtensionProperties[i].extensionName);
		OutputDebugString("\n");
		m_DeviceExtensionNames.push_back(_strdup(_vkExtensionProperties[i].extensionName));
	}

	return true;
}

bool CVulkanBase::InitializeGpu()
{
	//Enumerate the physical devices
	uint32_t gpuCount = 0;
	VK_RETURN_IF_FAILED(vkEnumeratePhysicalDevices(m_VulkanInstance, &gpuCount, nullptr));
	if (gpuCount > 0)
	{
		std::unique_ptr<VkPhysicalDevice[]> pGpuList(new VkPhysicalDevice[gpuCount]);
		VK_RETURN_IF_FAILED(vkEnumeratePhysicalDevices(m_VulkanInstance, &gpuCount, pGpuList.get()));
		m_VulkanGpu = pGpuList[0];
	}
	return true;
}

bool CVulkanBase::InitializeSurface()
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	info.hinstance = m_hInstance;
	info.hwnd = m_hWnd;
	VK_RETURN_IF_FAILED(vkCreateWin32SurfaceKHR(m_VulkanInstance, &info, nullptr, &m_VulkanSurface));
#endif

	return true;
}

bool CVulkanBase::InitializeInstance()
{
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 3);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "first";
	appInfo.pEngineName = "vulkan";

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = m_InstanceLayerNames.size();
	createInfo.ppEnabledLayerNames = m_InstanceLayerNames.data();
	createInfo.enabledExtensionCount = m_InstanceExtensionNames.size();
	createInfo.ppEnabledExtensionNames = m_InstanceExtensionNames.data();

	VK_RETURN_IF_FAILED(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance));

	return true;
}

bool CVulkanBase::InitializeInstanceLayerAndExt()
{
	//enumerate layer
	uint32_t _count = 0;
	VK_RETURN_IF_FAILED(vkEnumerateInstanceLayerProperties(&_count, nullptr));
	std::unique_ptr<VkLayerProperties[]> _vkLayerProperties(new VkLayerProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateInstanceLayerProperties(&_count, _vkLayerProperties.get()));
	for (auto i = 0u; i < _count; ++i)
	{
		OutputDebugString(_vkLayerProperties[i].layerName);
		OutputDebugString("     ");
		OutputDebugString(_vkLayerProperties[i].description);
		OutputDebugString("\n");
		m_InstanceLayerNames.push_back(_strdup(_vkLayerProperties[i].layerName));
	}
	//enumerate extension
	VK_RETURN_IF_FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &_count, nullptr));
	std::unique_ptr<VkExtensionProperties[]> _vkExtensionProperties(new VkExtensionProperties[_count]);
	VK_RETURN_IF_FAILED(vkEnumerateInstanceExtensionProperties(
		nullptr, &_count, _vkExtensionProperties.get()));
	for (auto i = 0u; i < _count; ++i)
	{
		OutputDebugString(_vkExtensionProperties[i].extensionName);
		OutputDebugString("\n");
		m_InstanceExtensionNames.push_back(_strdup(_vkExtensionProperties[i].extensionName));
	}

	return true;
}

void CVulkanBase::Run()
{

}

void CVulkanBase::Release()
{

}



