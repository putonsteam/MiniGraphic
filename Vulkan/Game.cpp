#include "Game.h"

bool CGame::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	if (!CVulkanBase::Initialize(hInstance, hWnd))
	{
		return false;
	}

	if (BeginCommandBuffer(m_VulkanCommandBuffer))
	{
		if (!LoadTexture())
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
	if (!LoadMeshData())
	{
		return false;
	}
	if (!loadShader())
	{
		return false;
	}
	if (!InilializePipeline())
	{
		return false;
	}
	if (!InilializeDescriptorSet())
	{
		return false;
	}
	for (int i = 0; i != m_uSwapchainImageCount; ++i)
	{
		if (!PrepareDraw(m_pRenderBufferArray[i]))
		{
			return false;
		}
	}

	return true;
}

bool CGame::PrepareDraw(RenderBuffer &buffer)
{
	BeginCommandBuffer(buffer.commandBuffer);

	// correspond the attachment(VkAttachmentDescription) of renderpass, 
	// if the loadOp is VK_ATTACHMENT_LOAD_OP_CLEAR
	VkClearValue clearValue[2] = {};
	clearValue[0].color.float32[0] = 1.0f;
	clearValue[0].color.float32[1] = 1.0f;
	clearValue[0].color.float32[2] = 1.0f;
	clearValue[0].color.float32[3] = 1.0f;
	clearValue[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo passInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	passInfo.clearValueCount = 2;
	passInfo.framebuffer = buffer.frameBuffer;
	passInfo.pClearValues = clearValue;
	passInfo.renderPass = m_VulkanRenderPass;
	passInfo.renderArea.offset.x = 0;
	passInfo.renderArea.offset.y = 0;
	passInfo.renderArea.extent.height = m_SurfaceExtent.height;
	passInfo.renderArea.extent.width = m_SurfaceExtent.width;

	vkCmdBeginRenderPass(buffer.commandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(buffer.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(buffer.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, NULL);

	VkViewport viewport = {};
	viewport.height = (float)m_SurfaceExtent.height;
	viewport.width = (float)m_SurfaceExtent.width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	vkCmdSetViewport(buffer.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = m_SurfaceExtent.width;
	scissor.extent.height = m_SurfaceExtent.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(buffer.commandBuffer, 0, 1, &scissor);

	vkCmdDraw(buffer.commandBuffer, 12 * 3, 1, 0, 0);
	vkCmdEndRenderPass(buffer.commandBuffer);

	VkImageMemoryBarrier prePresentBarrier =
	{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	prePresentBarrier.image = buffer.image;

	VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
	vkCmdPipelineBarrier(
		buffer.commandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0,
		NULL,
		0,
		NULL,
		1,
		pmemory_barrier);

	EndCommandBuffer(buffer.commandBuffer);

	return true;
}

void CGame::Run()
{	
	//continue until the device is idle
	vkDeviceWaitIdle(m_VulkanDevice);

	// Get the index of the next available swapchain image
	// so knows which framebuffer to use as a rendering target
	// he semaphore can be used to postpone the submission of the command buffer 
	// until the image is ready.
	vkAcquireNextImageKHR(m_VulkanDevice, m_VulkanSwapchain, UINT64_MAX,
		m_AcquireImageSemaphore, VK_NULL_HANDLE, &m_CurrentSwapBuffer);

	BeginCommandBuffer(m_VulkanCommandBuffer);
	SetImageLayout(
		m_VulkanCommandBuffer,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_pRenderBufferArray[m_CurrentSwapBuffer].image,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		(VkAccessFlagBits)0);
	EndCommandBuffer(m_VulkanCommandBuffer);
	FlushCommandBuffer(m_VulkanCommandBuffer);

	VkPipelineStageFlags _pipeline_staget_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_AcquireImageSemaphore;
	submitInfo.pWaitDstStageMask = &_pipeline_staget_flags;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_pRenderBufferArray[m_CurrentSwapBuffer].commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	// When the GPU is done executing the commands
	submitInfo.pSignalSemaphores = &m_RenderingDoneSemaphore;

	// You need to know when the GPU is done so that you don't start the present to the display too soon.
	vkQueueSubmit(m_VulkanQueue, 1, &submitInfo, m_pRenderBufferArray[m_CurrentSwapBuffer].fence);
	vkWaitForFences(m_VulkanDevice, 1, &m_pRenderBufferArray[m_CurrentSwapBuffer].fence,
		VK_TRUE, UINT64_MAX);
	vkResetFences(m_VulkanDevice, 1, &m_pRenderBufferArray[m_CurrentSwapBuffer].fence);

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	// When the GPU is done executing the commands
	presentInfo.pWaitSemaphores = &m_RenderingDoneSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_VulkanSwapchain;
	presentInfo.pImageIndices = &m_CurrentSwapBuffer;
	vkQueuePresentKHR(m_VulkanQueue, &presentInfo);
}

void CGame::Release()
{

}

CGame *CGame::GetInstance()
{
	static CGame m_instance;
	return &m_instance;
}

bool CGame::InilializePipeline()
{
	// create pipeline cache
	VkPipelineCacheCreateInfo cacheInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_RETURN_IF_FAILED(vkCreatePipelineCache(m_VulkanDevice, &cacheInfo,
		NULL, &m_PipelineCache));

	// hexue to
	VkDynamicState dynamic[100] = {};
	VkPipelineDynamicStateCreateInfo dynamicInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.pDynamicStates = dynamic;

	VkPipelineVertexInputStateCreateInfo vertexInput =
	{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkPipelineInputAssemblyStateCreateInfo InputAssembly =
	{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterize =
	{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterize.polygonMode = VK_POLYGON_MODE_FILL;
	rasterize.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterize.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterize.depthClampEnable = VK_FALSE;
	rasterize.rasterizerDiscardEnable = VK_FALSE;
	rasterize.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState blendAttach[1] = {};
	blendAttach[0].colorWriteMask = 0xf;
	blendAttach[0].blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo blendInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = blendAttach;

	VkPipelineViewportStateCreateInfo viewportInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportInfo.viewportCount = 1;
	dynamic[dynamicInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;

	viewportInfo.scissorCount = 1;
	dynamic[dynamicInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDepthStencilStateCreateInfo DepthStencilInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilInfo.depthTestEnable = VK_TRUE;
	DepthStencilInfo.depthWriteEnable = VK_TRUE;
	DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	DepthStencilInfo.back.failOp = VK_STENCIL_OP_KEEP;
	DepthStencilInfo.back.passOp = VK_STENCIL_OP_KEEP;
	DepthStencilInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	DepthStencilInfo.stencilTestEnable = VK_FALSE;
	DepthStencilInfo.front = DepthStencilInfo.back;

	VkPipelineMultisampleStateCreateInfo multiSampleInfo =
	{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multiSampleInfo.pSampleMask = NULL;
	multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineShaderStageCreateInfo shaderInfo[2] = {};
	shaderInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderInfo[0].module = m_ShaderVertex;
	shaderInfo[0].pName = "main";	// function access

	shaderInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderInfo[1].module = m_ShaderPix;
	shaderInfo[1].pName = "main";	// function access

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &InputAssembly;
	pipelineInfo.pRasterizationState = &rasterize;
	pipelineInfo.pColorBlendState = &blendInfo;
	pipelineInfo.pMultisampleState = &multiSampleInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pDepthStencilState = &DepthStencilInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderInfo;
	pipelineInfo.renderPass = m_VulkanRenderPass;
	pipelineInfo.pDynamicState = &dynamicInfo;

	VK_RETURN_IF_FAILED(vkCreateGraphicsPipelines(m_VulkanDevice, m_PipelineCache, 1,
		&pipelineInfo, nullptr, &m_Pipeline));

	return true;
}

bool CGame::InilializeDescriptorSet()
{
	VkDescriptorSetAllocateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	info.descriptorPool = m_DescriptorPool;
	info.descriptorSetCount = 1;
	info.pSetLayouts = &m_DescriptorSetLayout;

	VK_RETURN_IF_FAILED(vkAllocateDescriptorSets(m_VulkanDevice, &info, &m_DescriptorSet));

	VkWriteDescriptorSet write[2] = {};
	write[0].descriptorCount = 1;
	write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write[0].pBufferInfo = &m_VertexBuffer.bufferDescriptor;
	write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write[0].dstSet = m_DescriptorSet;

	write[1].descriptorCount = 1;
	write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write[1].pImageInfo = &m_TextureArray[0].imageDescriptor;
	write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write[1].dstSet = m_DescriptorSet;
	write[1].dstBinding = 1;

	vkUpdateDescriptorSets(m_VulkanDevice, 2, write, 0, nullptr);

	return true;
}

bool CGame::LoadMeshData()
{
	vec3 eye = { 0.0f, 3.0f, 5.0f };
	vec3 origin = { 0, 0, 0 };
	vec3 up = { 0.0f, 1.0f, 0.0 };

	mat4x4 projection;
	mat4x4 view;
	mat4x4 world;

	float aspect = m_SurfaceExtent.width / float(m_SurfaceExtent.height);
	mat4x4_perspective(projection, (float)degreesToRadians(45.0f), aspect, 0.1f, 100.0f);
	mat4x4_look_at(view, eye, origin, up);
	mat4x4_identity(world);

	struct vktexcube_vs_uniform {
		// Must start with MVP
		float mvp[4][4];
		float position[12 * 3][4];
		float attr[12 * 3][4];
	};
	mat4x4 MVP, VP;
	struct vktexcube_vs_uniform data;

	mat4x4_mul(VP, projection, view);
	mat4x4_mul(MVP, VP, world);
	memcpy(data.mvp, MVP, sizeof(MVP));
	//    dumpMatrix("MVP", MVP);

	static const float vertexData[] = {
		-1.0f, -1.0f, -1.0f,  // -X side
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,  // -Z side
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,  // -Y side
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,  // +Y side
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,

		1.0f, 1.0f, -1.0f,  // +X side
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, 1.0f, 1.0f,  // +Z side
		-1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
	};

	static const float uvData[] = {
		0.0f, 0.0f,  // -X side
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		1.0f, 0.0f,  // -Z side
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		1.0f, 1.0f,  // -Y side
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,

		1.0f, 1.0f,  // +Y side
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,

		1.0f, 1.0f,  // +X side
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,  // +Z side
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};


	for (auto i = 0; i < 12 * 3; i++) {
		data.position[i][0] = vertexData[i * 3];
		data.position[i][1] = vertexData[i * 3 + 1];
		data.position[i][2] = vertexData[i * 3 + 2];
		data.position[i][3] = 1.0f;
		data.attr[i][0] = uvData[2 * i];
		data.attr[i][1] = uvData[2 * i + 1];
		data.attr[i][2] = 0;
		data.attr[i][3] = 0;
	}

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.size = sizeof(data);
	VK_RETURN_IF_FAILED(vkCreateBuffer(m_VulkanDevice, &bufferInfo,
		NULL, &m_VertexBuffer.buffer));

	VkMemoryRequirements memoryReq;
	vkGetBufferMemoryRequirements(m_VulkanDevice, m_VertexBuffer.buffer, &memoryReq);

	if (!AllocMemory(memoryReq, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&m_VertexBuffer.memory))
	{
		return false;
	}
	uint8_t *pData = nullptr;
	VK_RETURN_IF_FAILED(vkMapMemory(m_VulkanDevice, m_VertexBuffer.memory,
		0, memoryReq.size, 0, (void **)&pData));

	memcpy(pData, &data, sizeof data);

	vkUnmapMemory(m_VulkanDevice, m_VertexBuffer.memory);

	VK_RETURN_IF_FAILED(vkBindBufferMemory(m_VulkanDevice, m_VertexBuffer.buffer,
		m_VertexBuffer.memory, 0));

	m_VertexBuffer.bufferDescriptor.buffer = m_VertexBuffer.buffer;
	m_VertexBuffer.bufferDescriptor.offset = 0;
	m_VertexBuffer.bufferDescriptor.range = sizeof(data);

	return true;
}

bool CGame::loadShader()
{
	VkShaderModule*		load[2] =
	{
		&m_ShaderVertex,
		&m_ShaderPix,
	};

	const char*			shaderName[2] =
	{
		"Shader/cube-vert.spv",
		"Shader/cube-frag.spv",
	};

	for (auto i = 0; i < 2; ++i)
	{
		FILE *fp = NULL;
		fopen_s(&fp, shaderName[i], "rb");
		assert(fp);
		fseek(fp, 0L, SEEK_END);
		long size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		std::unique_ptr<uint8_t[]> shader_code(new uint8_t[size]);
		fread_s(shader_code.get(), size, 1, size, fp);
		fclose(fp);

		VkShaderModuleCreateInfo moduleCreateInfo =
		{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shader_code.get();

		VK_RETURN_IF_FAILED(vkCreateShaderModule(m_VulkanDevice, &moduleCreateInfo, NULL, load[i]));
	}
	return true;
}

bool CGame::LoadTexture()
{
	std::string textureFile[] =
	{
		"source/Textures/bricks.ppm",
	};
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormatProperties	formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_VulkanGpu, format, &formatProperties);
	int32_t iWidth = 0;
	int32_t iHeight = 0;
	if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
	{
		for (auto &v : textureFile)
		{
			Texture tex = {};
			if (!ReadTextureFromFile(v.c_str(), NULL, NULL, &iWidth, &iHeight))
			{
				return false;
			}
			VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = format;
			imageInfo.extent.depth = 1;
			imageInfo.extent.width = (uint32_t)iWidth;
			imageInfo.extent.height = (uint32_t)iHeight;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.flags = 0;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

			VK_RETURN_IF_FAILED(vkCreateImage(m_VulkanDevice, &imageInfo, NULL, &tex.image));

			VkMemoryRequirements memoryReq;
			vkGetImageMemoryRequirements(m_VulkanDevice, tex.image, &memoryReq);
			// alloc memory 
			if (!AllocMemory(memoryReq, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &tex.memory))
			{
				return false;
			}

			VK_RETURN_IF_FAILED(vkBindImageMemory(m_VulkanDevice, tex.image, tex.memory, 0));

			//only when VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT is set
			VkImageSubresource sub = {};
			sub.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			sub.mipLevel = 0;
			sub.arrayLayer = 0;

			VkSubresourceLayout layout;
			void *_data;
			vkGetImageSubresourceLayout(m_VulkanDevice, tex.image, &sub, &layout);
			VK_RETURN_IF_FAILED(vkMapMemory(m_VulkanDevice, tex.memory, 0, memoryReq.size, 0, &_data));
			if (!ReadTextureFromFile(v.c_str(), (uint8_t*)_data, &layout, &iWidth, &iHeight)) {
				assert(0);
				return false;
			}
			vkUnmapMemory(m_VulkanDevice, tex.memory);

			SetImageLayout(
				m_VulkanCommandBuffer,
				VK_IMAGE_ASPECT_COLOR_BIT,
				tex.image,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	// use in shader
				VK_ACCESS_HOST_WRITE_BIT);

			//create sampler
			VkSamplerCreateInfo sampler = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
			sampler.magFilter = VK_FILTER_NEAREST;
			sampler.minFilter = VK_FILTER_NEAREST;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.mipLodBias = 0.0f;
			sampler.anisotropyEnable = VK_FALSE;
			sampler.maxAnisotropy = 1;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			sampler.maxLod = 0.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			sampler.unnormalizedCoordinates = VK_FALSE;
			/* create sampler */
			VK_RETURN_IF_FAILED(vkCreateSampler(m_VulkanDevice, &sampler,
				NULL, &tex.sampler));

			VkImageViewCreateInfo _view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			_view.image = tex.image;
			_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			_view.format = format;
			_view.components.a = VK_COMPONENT_SWIZZLE_A;
			_view.components.r = VK_COMPONENT_SWIZZLE_R;
			_view.components.g = VK_COMPONENT_SWIZZLE_G;
			_view.components.b = VK_COMPONENT_SWIZZLE_B;
			_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			_view.subresourceRange.baseMipLevel = 0;
			_view.subresourceRange.levelCount = 1;
			_view.subresourceRange.baseArrayLayer = 0;
			_view.subresourceRange.layerCount = 1;
			/* create image view */
			VK_RETURN_IF_FAILED(vkCreateImageView(m_VulkanDevice, &_view, NULL, &tex.imageView));

			tex.imageDescriptor.sampler = tex.sampler;
			tex.imageDescriptor.imageView = tex.imageView;
			tex.imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			m_TextureArray.push_back(tex);
		}
		return true;
	}
	return false;
}

/* Load a ppm file into memory */
bool CGame::ReadTextureFromFile(const char *filename, uint8_t *rgba_data,
	VkSubresourceLayout *layout, int32_t *width, int32_t *height) {

	FILE *fPtr = NULL;
	fopen_s(&fPtr, filename, "rb");
	char header[256], *cPtr, *tmp;

	if (!fPtr)
		return false;

	cPtr = fgets(header, 256, fPtr); // P6
	if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
		fclose(fPtr);
		return false;
	}

	do {
		cPtr = fgets(header, 256, fPtr);
		if (cPtr == NULL) {
			fclose(fPtr);
			return false;
		}
	} while (!strncmp(header, "#", 1));

	sscanf_s(header, "%u %u", height, width);
	if (rgba_data == NULL) {
		fclose(fPtr);
		return true;
	}
	tmp = fgets(header, 256, fPtr); // Format
	(void)tmp;
	if (cPtr == NULL || strncmp(header, "255\n", 3)) {
		fclose(fPtr);
		return false;
	}

	for (int y = 0; y < *height; y++) {
		uint8_t *rowPtr = rgba_data;
		for (int x = 0; x < *width; x++) {
			size_t s = fread(rowPtr, 3, 1, fPtr);
			(void)s;
			rowPtr[3] = 255; /* Alpha of 1 */
			rowPtr += 4;
		}
		rgba_data += layout->rowPitch;
	}
	fclose(fPtr);
	return true;
}
