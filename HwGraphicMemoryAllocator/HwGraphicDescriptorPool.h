//******************************************************************************
//	@file	HwGraphicDescriptors.cpp
//	@brief	NVN Graphic Descriptors
//	@author	Jiang Hong
//******************************************************************************
#ifndef _HW_GRAPHIC_DESCRIPTORS_H_
#define _HW_GRAPHIC_DESCRIPTORS_H_

#include <Hw/Container/HwFixedVector.h>

namespace Hw
{
    class cGraphicDescriptorPool;

    enum class GRAPHIC_DESCRIPTOR_TYPE
    {
        UNKNOW,
        SAMPLED_TEXTURE,
        STORAGE_TEXTURE,
        UNIFORM_BUFFER,
        STORAGE_BUFFER,
        SAMPLER,
    };

    class cGraphicDescriptor
    {
    public:
        friend class cGraphicDescriptorPool;

        cGraphicDescriptor()
            : m_Type(GRAPHIC_DESCRIPTOR_TYPE::UNKNOW)
        {
            memset(&m_Descriptor, 0, sizeof(m_Descriptor));
        }
        
        void updateBuffer(nvn::BufferAddress Address, size_t Size)
        {
            HW_ASSERT(m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNIFORM_BUFFER || m_Type == GRAPHIC_DESCRIPTOR_TYPE::STORAGE_BUFFER);
            if (m_Type == GRAPHIC_DESCRIPTOR_TYPE::UNIFORM_BUFFER)
            {
                m_Descriptor.UniformBuffer.Address = Address;
                m_Descriptor.UniformBuffer.Size = Size;
            }
            else if (m_Type == GRAPHIC_DESCRIPTOR_TYPE::STORAGE_BUFFER)
            {
                m_Descriptor.StroageBuffer.Address = Address;
                m_Descriptor.StroageBuffer.Size = Size;
            }
        }

        inline bool isVaild() const { return m_Type != GRAPHIC_DESCRIPTOR_TYPE::UNKNOW; }
        inline GRAPHIC_DESCRIPTOR_TYPE getType() const { return m_Type; }

        nvn::SeparateTextureHandle getSeparateTextureHandle() const;
        nvn::SeparateSamplerHandle getSeparateSamplerHandle() const;
        nvn::ImageHandle getImageHandle() const;
        nvn::BufferAddress getUniformBufferAddress() const;
        size_t getUniformBufferSize() const;
        nvn::BufferAddress getStorageBufferAddress() const;
        size_t getStorageBufferSize() const;

    private:

        struct SampledTextureDescriptor
        {
            nvn::SeparateTextureHandle Handle;
            u32 Id;
        };

        struct StorageTextureDescriptor
        {
            nvn::ImageHandle Handle;
            u32 Id;
        };

        struct BufferDescriptor
        {
            nvn::BufferAddress Address;
            size_t Size;
        };

        struct SamplerDescriptor
        {
            nvn::SeparateSamplerHandle Handle;
            u32 Id;
        };

        GRAPHIC_DESCRIPTOR_TYPE m_Type;
        union 
        {
            SampledTextureDescriptor SampledTexture;
            StorageTextureDescriptor StorageTexture;
            BufferDescriptor UniformBuffer;
            BufferDescriptor StroageBuffer;
            SamplerDescriptor Sampler;
        } m_Descriptor;

    };

    class cGraphicDescriptorPool
    {
        friend class cGraphicObjectFactory;
    public:

        void release(void);

        b32 create(nvn::Device* pDevice, cGraphicMemoryAllocator* pGraphicMemoryAllocator, const cGraphicSetupInfo& SetupInfo);

        b32 getNewTextureId(u32& rID);

        b32 getNewSamplerId(u32& rID);

        b32 pushIdToRecyle(const cGraphicDescriptor& rDescriptor);

        nvn::TexturePool* getTexturePool() { return &m_TexturePool; }
        nvn::SamplerPool* getSamplerPool() { return &m_SamplerPool; }

        bool registerSampledTexture(cGraphicDescriptor* pDescriptor, const nvn::Texture* pTexture, const nvn::TextureView* pTextureView = nullptr);
        bool registerStorageTexture(cGraphicDescriptor* pDescriptor, const nvn::Texture* pTexture, const nvn::TextureView* pTextureView = nullptr);
        bool registerUniformBuffer(cGraphicDescriptor* pDescriptor, const nvn::Buffer* pBuffer, size_t Offset, size_t BufferSize);
        bool registerStorageBuffer(cGraphicDescriptor* pDescriptor, const nvn::Buffer* pBuffer, size_t Offset, size_t BufferSize);
        bool registerSampler(cGraphicDescriptor* pDescriptor, const nvn::Sampler* pBuffer);

    private:
        cGraphicDescriptorPool();
        ~cGraphicDescriptorPool();
        void clear(void);

        b32 pushTexureIDToRecyle(const u32 ID);
        b32 pushSamplerIDToRecyle(const u32 ID);

    private:

        cHeap* m_pWorkHeap;
        nvn::Device* m_pDevice;

        nvn::TexturePool m_TexturePool;
        nvn::SamplerPool m_SamplerPool;

        int             m_NewTextureId;
        int             m_NewSamplerId;

        cFixedVector<u32> m_FreeTextureIDStack;
        cFixedVector<u32> m_FreeSamplerIDStack;

        cGraphicMemoryAllocation m_Allocation;
    };
}

#endif //_HW_GRAPHIC_DESCRIPTORS_H_