//******************************************************************************
//	@file	HwGraphicDescriptorPool.cpp
//	@brief	NVN Graphic Descriptors
//	@author	Jiang Hong
//******************************************************************************

#include "HwGraphicDescriptorPool.h"

using namespace Hw;

nvn::SeparateTextureHandle cGraphicDescriptor::getSeparateTextureHandle() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::SAMPLED_TEXTURE);
    return m_Descriptor.SampledTexture.Handle;
}

nvn::SeparateSamplerHandle cGraphicDescriptor::getSeparateSamplerHandle() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::SAMPLER);
    return m_Descriptor.Sampler.Handle;
}

nvn::ImageHandle cGraphicDescriptor::getImageHandle() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::SAMPLED_TEXTURE);
    return m_Descriptor.StorageTexture.Handle;
}

nvn::BufferAddress cGraphicDescriptor::getUniformBufferAddress() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNIFORM_BUFFER);
    return m_Descriptor.UniformBuffer.Address;
}

size_t cGraphicDescriptor::getUniformBufferSize() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNIFORM_BUFFER);
    return m_Descriptor.UniformBuffer.Size;
}

nvn::BufferAddress cGraphicDescriptor::getStorageBufferAddress() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::STORAGE_BUFFER);
    return m_Descriptor.StroageBuffer.Address;
}

size_t cGraphicDescriptor::getStorageBufferSize() const
{
    HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::STORAGE_BUFFER);
    return m_Descriptor.StroageBuffer.Size;
}


// Virtuos JH : now we use default texture pool, maybe will use more texture pool later.
static const uint32_t s_DefaultNumTextures = 4096;
static const uint32_t s_DefaultNumSamplers = 2048;
static const size_t s_DefaultAlignment = 4;


cGraphicDescriptorPool::cGraphicDescriptorPool()
{
    clear();
}

cGraphicDescriptorPool::~cGraphicDescriptorPool()
{
    release();
}

b32 cGraphicDescriptorPool::create(nvn::Device* pDevice, cGraphicMemoryAllocator* pGraphicMemoryAllocator, const cGraphicSetupInfo& SetupInfo)
{
    
    HW_ASSERT(pGraphicMemoryAllocator != nullptr);
    HW_ASSERT(pDevice !=nullptr);

    m_pWorkHeap = SetupInfo.m_pWorkHeap;
    m_pDevice = pDevice;

    m_FreeTextureIDStack.create(s_DefaultNumTextures, *m_pWorkHeap);
    m_FreeSamplerIDStack.create(s_DefaultNumSamplers, *m_pWorkHeap);

    //Virtuos JH: init texture & sampler pool
    {

        m_NewTextureId = cGraphicDeviceImpl::NumReservedTextureDescriptors;
        m_NewSamplerId = cGraphicDeviceImpl::NumReservedSamplerDescriptors;

        int totalNumTextureDescriptors = s_DefaultNumTextures + cGraphicDeviceImpl::NumReservedTextureDescriptors;
        int totalNumSamplerDescriptors = s_DefaultNumSamplers + cGraphicDeviceImpl::NumReservedSamplerDescriptors;

        size_t textureDescriptorAreaSize = totalNumTextureDescriptors * cGraphicDeviceImpl::TextureDescriptorSize;
        size_t samplerDescriptorAreaSize = totalNumSamplerDescriptors * cGraphicDeviceImpl::SamplerDescriptorSize;

        HW_ASSERT(totalNumTextureDescriptors <= cGraphicDeviceImpl::MaxTextureDescriptorsInPool);
        HW_ASSERT(totalNumSamplerDescriptors <= cGraphicDeviceImpl::MaxSamplerDescriptorsInPool);

        {
            //Virtuos: JH alloc memory
            size_t DescriptorsMemPoolSize = textureDescriptorAreaSize + samplerDescriptorAreaSize;
            GRAPHIC_MEMORY_ERROR error = pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::SAMPLED_TEXTURE, DescriptorsMemPoolSize, s_DefaultAlignment, &m_Allocation);
            HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);
            ptrdiff_t textureDescriptorsOffset = m_Allocation.getOffset();
            ptrdiff_t samplerDescriptorsOffset = textureDescriptorsOffset + textureDescriptorAreaSize;

            {
                if (!m_TexturePool.Initialize(m_Allocation.getNVNMemoryPool(), textureDescriptorsOffset, totalNumTextureDescriptors))
                {
                    LogAssert("nvnTexturePoolInitialize failed.\n");
                }

                if (!m_SamplerPool.Initialize(m_Allocation.getNVNMemoryPool(), samplerDescriptorsOffset, totalNumSamplerDescriptors))
                {
                    LogAssert("nvnSamplerPoolInitialize failed.\n");
                }
            }
        }
    }

    return TRUE;
}

void cGraphicDescriptorPool::clear(void)
{
    m_NewTextureId = 0;
    m_NewSamplerId = 0;

    m_pWorkHeap = nullptr;
    m_pDevice = nullptr;
}

void cGraphicDescriptorPool::release(void)
{
    m_Allocation.release();
    m_SamplerPool.Finalize();
    m_TexturePool.Finalize();

    m_FreeTextureIDStack.clear();
    m_FreeSamplerIDStack.clear();
}

b32 cGraphicDescriptorPool::getNewTextureId(u32& rID)
{
    int LastVailedIndex = m_FreeTextureIDStack.getSize() - 1;

    // Virtuos : JH No free texture id
    if (LastVailedIndex < 0)
    {
        rID = m_NewTextureId++;
    }
    // return id from recycle stack
    else
    {
        rID = m_FreeTextureIDStack[LastVailedIndex];
        m_FreeTextureIDStack.erase(LastVailedIndex);
    }
    return TRUE;
}

b32 cGraphicDescriptorPool::getNewSamplerId(u32& rID)
{
    int LastVailedIndex = m_FreeSamplerIDStack.getSize() - 1;

    // Virtuos : JH No free sampler id
    if (LastVailedIndex < 0)
    {
        rID = m_NewSamplerId++;
    }
    // return id from recycle stack
    else
    {
        rID = m_FreeSamplerIDStack[LastVailedIndex];
        m_FreeSamplerIDStack.erase(LastVailedIndex);
    }
    return TRUE;
}

b32 cGraphicDescriptorPool::pushIdToRecyle(const cGraphicDescriptor& rDescriptor)
{
    HW_ASSERT(rDescriptor.isVaild());
    switch (rDescriptor.getType())
    {
    case GRAPHIC_DESCRIPTOR_TYPE::SAMPLED_TEXTURE:
    {
        return pushTexureIDToRecyle(rDescriptor.m_Descriptor.SampledTexture.Id);
    }
    case GRAPHIC_DESCRIPTOR_TYPE::STORAGE_TEXTURE:
    {
        return pushTexureIDToRecyle(rDescriptor.m_Descriptor.StorageTexture.Id);
    }
    case GRAPHIC_DESCRIPTOR_TYPE::SAMPLER:
    {
        return pushTexureIDToRecyle(rDescriptor.m_Descriptor.Sampler.Id);
    }
    default:
        HW_ASSERT(false);
        break;
    }

    return false;
}

b32 cGraphicDescriptorPool::pushTexureIDToRecyle(const u32 rID)
{
    if (!m_FreeTextureIDStack.canAdd())
    {
        return FALSE;
    }

    m_FreeTextureIDStack.pushBack(rID);

    return TRUE;
}

b32 cGraphicDescriptorPool::pushSamplerIDToRecyle(const u32 rID)
{
    if (!m_FreeSamplerIDStack.canAdd())
    {
        return FALSE;
    }

    m_FreeSamplerIDStack.pushBack(rID);

    return TRUE;
}

bool cGraphicDescriptorPool::registerSampledTexture(cGraphicDescriptor* pDescriptor, const nvn::Texture* pTexture, const nvn::TextureView* pTextureView /*= nullptr*/)
{
    HW_ASSERT(pDescriptor);
    HW_ASSERT(pTexture);
    HW_ASSERT(pDescriptor->m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNKNOW);
    
    cGraphicDescriptor::SampledTextureDescriptor& SampledTexture = pDescriptor->m_Descriptor.SampledTexture;

    if (!getNewTextureId(SampledTexture.Id))
    {
        return false;
    }

    m_TexturePool.RegisterTexture(SampledTexture.Id, pTexture, pTextureView);

    pDescriptor->m_Type = GRAPHIC_DESCRIPTOR_TYPE::SAMPLED_TEXTURE;

    SampledTexture.Handle = m_pDevice->GetSeparateTextureHandle(SampledTexture.Id);

    return true;
}

bool cGraphicDescriptorPool::registerStorageTexture(cGraphicDescriptor* pDescriptor, const nvn::Texture* pTexture, const nvn::TextureView* pTextureView /*= nullptr*/)
{
    HW_ASSERT(pDescriptor);
    HW_ASSERT(pTexture);
    HW_ASSERT(pDescriptor->m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNKNOW);

    cGraphicDescriptor::StorageTextureDescriptor& StorageTexture = pDescriptor->m_Descriptor.StorageTexture;

    if (!getNewTextureId(StorageTexture.Id))
    {
        return false;
    }

    m_TexturePool.RegisterImage(StorageTexture.Id, pTexture, pTextureView);

    pDescriptor->m_Type = GRAPHIC_DESCRIPTOR_TYPE::STORAGE_TEXTURE;

    StorageTexture.Handle = m_pDevice->GetImageHandle(StorageTexture.Id);

    return true;
}

bool cGraphicDescriptorPool::registerUniformBuffer(cGraphicDescriptor* pDescriptor, const nvn::Buffer* pBuffer, size_t Offset, size_t BufferSize)
{
    HW_ASSERT(pDescriptor);
    HW_ASSERT(pBuffer);
    HW_ASSERT(pDescriptor->m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNKNOW);

    cGraphicDescriptor::BufferDescriptor& BufferDescriptor = pDescriptor->m_Descriptor.UniformBuffer;
    pDescriptor->m_Type = GRAPHIC_DESCRIPTOR_TYPE::UNIFORM_BUFFER;

    HW_ASSERT(BufferSize + Offset <= pBuffer->GetSize());
    BufferDescriptor.Address = pBuffer->GetAddress() + Offset;
    BufferDescriptor.Size = BufferSize;

    return true;
}

bool cGraphicDescriptorPool::registerStorageBuffer(cGraphicDescriptor* pDescriptor, const nvn::Buffer* pBuffer, size_t Offset, size_t BufferSize)
{
    HW_ASSERT(pDescriptor);
    HW_ASSERT(pBuffer);
    HW_ASSERT(pDescriptor->m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNKNOW);

    cGraphicDescriptor::BufferDescriptor& BufferDescriptor = pDescriptor->m_Descriptor.StroageBuffer;
    pDescriptor->m_Type = GRAPHIC_DESCRIPTOR_TYPE::STORAGE_BUFFER;

    HW_ASSERT(BufferSize + Offset <= pBuffer->GetSize());
    BufferDescriptor.Address = pBuffer->GetAddress() + Offset;
    BufferDescriptor.Size = BufferSize;

    return true;
}

bool cGraphicDescriptorPool::registerSampler(cGraphicDescriptor* pDescriptor, const nvn::Sampler* pSampler)
{
    HW_ASSERT(pDescriptor);
    HW_ASSERT(pSampler);
    HW_ASSERT(pDescriptor->m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNKNOW);

    cGraphicDescriptor::SamplerDescriptor& SamplerDescriptor = pDescriptor->m_Descriptor.Sampler;

    if (!getNewSamplerId(SamplerDescriptor.Id))
    {
        return false;
    }

    m_SamplerPool.RegisterSampler(SamplerDescriptor.Id, pSampler);

    pDescriptor->m_Type = GRAPHIC_DESCRIPTOR_TYPE::SAMPLER;

    SamplerDescriptor.Handle = m_pDevice->GetSeparateSamplerHandle(SamplerDescriptor.Id);

    return true;
}

//******************************************************************************
//	End of File
//******************************************************************************