#pragma once

#include "VulkanBase.h"

class CGame : public CVulkanBase
{
public:
	struct Texture
	{
		VkImage image;
		VkImageView imageView;
		VkSampler sampler;
		VkDeviceMemory memory;
		VkDescriptorImageInfo imageDescriptor;
	};

	struct VertexBuffer
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo bufferDescriptor;
	};

	virtual bool Initialize(HINSTANCE hInstance, HWND hWnd) override;
	virtual void Run() override;
	virtual void Release() override;
	static CGame *GetInstance();

private:
	bool PrepareDraw(RenderBuffer &buffer);
	bool InilializeDescriptorSet();
	bool InilializePipeline();
	bool LoadMeshData();
	bool loadShader();
	bool LoadTexture();
	bool ReadTextureFromFile(const char *filename, uint8_t *rgba_data,
		VkSubresourceLayout *layout, int32_t *width, int32_t *height);

private:
	VkPipeline m_Pipeline;
	VkPipelineCache	m_PipelineCache;
	VkDescriptorSet m_DescriptorSet;	// fill data to describe 
	std::vector<Texture> m_TextureArray;
	VkShaderModule m_ShaderVertex;
	VkShaderModule m_ShaderPix;
	VertexBuffer m_VertexBuffer;
};