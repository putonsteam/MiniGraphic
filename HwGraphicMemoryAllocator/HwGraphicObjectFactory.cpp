//******************************************************************************
//	@file	HwGraphicObjectFactory.cpp
//	@brief	グラフィック・オブジェクト・ファクトリー( PS4 )
//	@author	Tsuyoshi Odera.
//******************************************************************************
#include <Hw/Hw.h>
#include "HwGraphicObjectFactory.h"
#include "equdef.h"

//******************************************************************************
//	デフォルトネームスペース
//******************************************************************************
using namespace Hw;

//------------------------------------------------------------------------------
//!	コンストラクタ
//------------------------------------------------------------------------------
cGraphicObjectFactory::cGraphicObjectFactory()
{
	clear();
}

//------------------------------------------------------------------------------
//!	デストラクタ
//------------------------------------------------------------------------------
cGraphicObjectFactory::~cGraphicObjectFactory()
{
	release();
}

//------------------------------------------------------------------------------
//!	クリア
//------------------------------------------------------------------------------
void cGraphicObjectFactory::clear( void )
{
    m_pBorderColor      = nullptr;
    m_BorderColorNum    = 0;
    m_pDevice           = nullptr;
    m_pWorkHeap         = nullptr;
    m_pBufferHeap       = nullptr;
    m_pVramHeap         = nullptr;
}

//------------------------------------------------------------------------------
//!	開放
//------------------------------------------------------------------------------
void cGraphicObjectFactory::release ( void )
{
    if (m_pBorderColor != nullptr) delete[](m_pBorderColor);

    clear();
}

//------------------------------------------------------------------------------
//!	生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::create (nvn::Device *pDevice, cGraphicMemoryAllocator* pGraphicMemoryAllocator, const cGraphicSetupInfo &SetupInfo )
{
    //==========================================================================
    // サンプラーステートのボーダーカラー情報作成
    //==========================================================================
    {
        // ボーダーカラーの設定
        if ((SetupInfo.m_pBorderColor != nullptr) && (SetupInfo.m_BorderColorNum > 0))
        {
            m_pBorderColor = new((*SetupInfo.m_pWorkHeap))float[SetupInfo.m_BorderColorNum * 4];

            if (m_pBorderColor == nullptr) return FALSE;

            for (u32 i = 0; i < SetupInfo.m_BorderColorNum; i++)
            {
                const cVec4& BorderColor = SetupInfo.m_pBorderColor[i];
                m_pBorderColor[i * 4 + 0] = BorderColor.x;
                m_pBorderColor[i * 4 + 1] = BorderColor.y;
                m_pBorderColor[i * 4 + 2] = BorderColor.z;
                m_pBorderColor[i * 4 + 3] = BorderColor.w;
            }

            m_BorderColorNum = SetupInfo.m_BorderColorNum;
        }
    }

    m_pDevice                   = pDevice;
    m_pGraphicMemoryAllocator   = pGraphicMemoryAllocator;
    m_BorderColorNum            = SetupInfo.m_BorderColorNum;
    m_pWorkHeap                 = SetupInfo.m_pWorkHeap;
    m_pBufferHeap               = SetupInfo.m_pBufferHeap;
    m_pVramHeap                 = SetupInfo.m_pVramHeap;

    if (m_pDevice==nullptr)
    {
        return FALSE;
    }

    if (m_pGraphicMemoryAllocator == nullptr)
    {
        return FALSE;
    }

    m_GraphicDescriptors.create(m_pDevice, m_pGraphicMemoryAllocator, SetupInfo);

    return TRUE;
}

//------------------------------------------------------------------------------
//! クリアバッファ・深度・ステートの設定
//------------------------------------------------------------------------------
void cGraphicObjectFactory::setClearBuffer_DepthState ( DEPTH_STENCIL_STATE_DESC &DepthDesc, b32 Enable )
{
	DepthDesc.DepthEnable		= TRUE;
	DepthDesc.DepthFunc			= GS_COMPARISON_ALWAYS;

	if( Enable )	DepthDesc.DepthWriteEnable	= TRUE;
	else			DepthDesc.DepthWriteEnable	= FALSE;
}

//------------------------------------------------------------------------------
//! クリアバッファ・ステンシル・ステートの設定
//------------------------------------------------------------------------------
void cGraphicObjectFactory::setClearBuffer_StencilState ( DEPTH_STENCIL_STATE_DESC &DepthDesc, b32 Enable )
{
	if( Enable ){
		DepthDesc.StencilMode					= GS_STENCIL_MODE_NORMAL;
		DepthDesc.FrontFaceOp.StencilFunc		= GS_COMPARISON_ALWAYS;
		DepthDesc.FrontFaceOp.StencilFailOp		= GS_STENCIL_OP_REPLACE;
		DepthDesc.FrontFaceOp.DepthFailOp		= GS_STENCIL_OP_REPLACE;
		DepthDesc.FrontFaceOp.StencilPassOp		= GS_STENCIL_OP_REPLACE;
		DepthDesc.StencilReadMask				= 0xFF;
		DepthDesc.StencilWriteMask				= 0xFF;
	} else {
		DepthDesc.StencilMode				= GS_STENCIL_MODE_INVALID;
		DepthDesc.FrontFaceOp.StencilFunc	= GS_COMPARISON_NEVER;
		DepthDesc.FrontFaceOp.StencilFailOp	= GS_STENCIL_OP_KEEP;
		DepthDesc.FrontFaceOp.DepthFailOp	= GS_STENCIL_OP_KEEP;
		DepthDesc.FrontFaceOp.StencilPassOp	= GS_STENCIL_OP_KEEP;
		DepthDesc.StencilReadMask			= 0;
		DepthDesc.StencilWriteMask			= 0;
	}

	DepthDesc.StencilRef		= 0;
}

//------------------------------------------------------------------------------
//! クリアバッファ・オブジェクトの生成 
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createClearBufferObject ( cClearBufferObjectImpl &oObject, const cClearBufferShaderInfo &ShaderInfo )
{
	NOT_IMPLEMENTED();
	
	return TRUE;
}

//------------------------------------------------------------------------------
//! リゾルブ・オブジェクトの生成 
//! @param[out]	oObject		リゾルブオブジェクト
//! @param[in]	ShaderInfo	シェーダー情報
//! @retval		TRUE		成功
//! @retval		FALSE		失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createResolveObject ( cResolveObjectImpl &oObject, const cResolveShaderInfo &ShaderInfo )
{
	static const VERTEX_LAYOUT_ELEMENT Element[]={
		{ 0, 0, VERTEX_LAYOUT_TYPE_FLOAT4, VERTEX_SEMANTIC_POSITION, 0 },
		VERTEX_LAYOUT_END()
	};
	
	oObject.m_pVertexBuffer		= new( ( *m_pWorkHeap ) )cVertexBufferImpl();
	if( oObject.m_pVertexBuffer == NULL ) return FALSE;

	oObject.m_pVertexShader		= new( ( *m_pWorkHeap ) )cVertexShaderImpl();
	if( oObject.m_pVertexShader == NULL ) return FALSE;

	oObject.m_pPixelShader		= new( ( *m_pWorkHeap ) )cPixelShaderImpl();
	if( oObject.m_pPixelShader == NULL ) return FALSE;

	oObject.m_pComputeShader	= new( ( *m_pWorkHeap ) )cComputeShaderImpl();
	if( oObject.m_pComputeShader == NULL ) return FALSE;

	oObject.m_pVertexLayout		= new( ( *m_pWorkHeap ) )cVertexLayoutImpl();
	if( oObject.m_pVertexLayout == NULL ) return FALSE;

	Hw::CREATE_VERTEX_SHADER_INFO info;
	info.pData = ShaderInfo.m_pVertexShader;
	if( !createVertexShader ( ( *oObject.m_pVertexShader ), info ) )
	{
		return FALSE;
	}

	if( !createPixelShader ( ( *oObject.m_pPixelShader ), ShaderInfo.m_pPixelShader ) )
	{
		return FALSE;
	}

	if( !createComputeShader ( ( *oObject.m_pComputeShader ), ShaderInfo.m_pComputeShader ) )
	{
		return FALSE;
	}

	if( !createVertexLayout( ( *oObject.m_pVertexLayout ), Element, ARRAY_SIZE( Element ) ) )
	{
		return FALSE;
	}

	{	// 頂点データの作成
		cVec4 s_VertexTbl[ 4 ] = 
		{
			cVec4( 0.0f, 0.0f, 0.0f, 1.0 ), 
			cVec4( 1.0f, 0.0f, 0.0f, 1.0 ), 
			cVec4( 0.0f, 1.0f, 0.0f, 1.0 ),
			cVec4( 1.0f, 1.0f, 0.0f, 1.0 ),
		};
	
		if( !createVertexBuffer ( ( *oObject.m_pVertexBuffer ), s_VertexTbl, ( *oObject.m_pVertexLayout ), 0, 4, m_pBufferHeap ) )
		{
			return FALSE;
		}
	}

	oObject.m_BufferNo	= ShaderInfo.m_BufferNo;
	oObject.m_Initalize	= TRUE;

	return TRUE;
}

//------------------------------------------------------------------------------
//!	深度サーフェイスの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createDepthSurface ( cDepthSurfaceImpl &Surface, const CREATE_DEPTH_SURFACE_INFO &Info )
{
    cTextureImpl* pTexture = new(*m_pWorkHeap) cTextureImpl();
    HW_ASSERT(pTexture != nullptr && "cGraphicObjectFactory::createDepthSurface pTexture == nullpter");

    nvn::TextureBuilder DepthStencilBuilder;
    DepthStencilBuilder.SetDefaults()
        .SetDevice(m_pDevice)
        .SetSize2D(Info.Width, Info.Height)
        .SetTarget(nvn::TextureTarget::TARGET_2D)
        .SetFlags(nvn::TextureFlags::COMPRESSIBLE)
        .SetFormat(Info.UseStencil?nvn::Format::DEPTH32F_STENCIL8:nvn::Format::DEPTH32F);
    if (Info.SamplingType > GD_MULTI_SAMPLING_TYPE_01)
    {
        DepthStencilBuilder.SetSamples(Info.SamplingType == GD_MULTI_SAMPLING_TYPE_02 ? 2 : 4);
    }
    size_t depthTextureSize = DepthStencilBuilder.GetStorageSize();
    size_t depthTextureAlignment = DepthStencilBuilder.GetStorageAlignment();
    HW_UNUSED_VAR(depthTextureAlignment);

    pTexture->m_pNVNTexture = new(*m_pWorkHeap) nvn::Texture();
    HW_ASSERT(pTexture->m_pNVNTexture != nullptr);

    if (!pTexture->initNvnTexture(*m_pGraphicMemoryAllocator, nullptr, depthTextureSize, GRAPHIC_MEMORY_RESOURCE_USAGE::DEPTH_STENCIL, DepthStencilBuilder))
    {
        LogAssert("cGraphicObjectFactory::createDepthSurface depthSurfaceTexture init failed!");
    }
    if (!m_GraphicDescriptors.registerSampledTexture(&pTexture->getSampledTextureDescriptor(), pTexture->m_pNVNTexture))
    {
        LogAssert("cGraphicObjectFactory::createDepthSurface depthSurfaceTexture register failed!");
    }

    Surface.m_pInstance = pTexture;

    Surface.m_Width = Info.Width;
    Surface.m_Height = Info.Height;
    Surface.m_SamplingType = Info.SamplingType;

	return TRUE;
}

//------------------------------------------------------------------------------
//! ロック可能テクスチャ作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createLockableTexture ( cTextureImpl &Texture, const CREATE_LOCKABLE_TEXTURE_INFO &Info )
{
    u32 Width = 0;
    u32 Height = 0;
    u32 Depth = 0;
    nvn::Format Format;

    nvn::TextureBuilder TextureBuilder;
    TextureBuilder.SetDefaults();

    Format = GraphicUtilityImpl::ConvertLockableFormat(Info.Type);
    bool Is3D = (Info.Depth != 0);

    Width = Info.Width;
    Height = Info.Height;
    Depth = Is3D ? Info.Depth : 1;

    TextureBuilder.SetTarget(Is3D ? nvn::TextureTarget::TARGET_3D : nvn::TextureTarget::TARGET_2D)
                  .SetSize3D(Width, Height, Depth)
                  .SetFormat(Format)
                  .SetDevice(m_pDevice)
                  .SetFlags(nvn::TextureFlags::LINEAR) // A lockable texture should be a linear texture.
                  .SetStride(GraphicUtilityImpl::CalculateStride(Width, Format));

    if (Info.Type == LOCKABLE_TEXTURE_TYPE_MOVIE_YUV)
    {
        // nvn does not support A8 texture format, we use swizzle to make the alpha channel work correctly.
        TextureBuilder.SetSwizzle(nvn::TextureSwizzle::ZERO, nvn::TextureSwizzle::ZERO, nvn::TextureSwizzle::ZERO, nvn::TextureSwizzle::R);
    }
    else if (Info.Type == LOCKABLE_TEXTURE_TYPE_MOVIE_RGBA)
    {
        // The real texture pixle format is ARGB8, we remap channels with swizzle.
        TextureBuilder.SetSwizzle(nvn::TextureSwizzle::B, nvn::TextureSwizzle::G, nvn::TextureSwizzle::R, nvn::TextureSwizzle::A);
    }

    GRAPHIC_MEMORY_RESOURCE_USAGE ResourceUsage = GraphicUtilityImpl::ConvertLockableUsage(Info.Type);
    size_t TextureSize = TextureBuilder.GetStorageSize();
    size_t TextureAlignment = TextureBuilder.GetStorageAlignment();

    Texture.m_pNVNTexture = new(*m_pWorkHeap) nvn::Texture();
    HW_ASSERT(Texture.m_pNVNTexture != nullptr);
    HW_UNUSED_VAR(TextureAlignment);

    if (!Texture.initNvnTexture(*m_pGraphicMemoryAllocator, nullptr, TextureSize, ResourceUsage, TextureBuilder))
    {
        LogAssert("cGraphicObjectFactory::createTargetTextureImp nvn texture init failed!");
    }
    if (!m_GraphicDescriptors.registerSampledTexture(&Texture.getSampledTextureDescriptor(), Texture.m_pNVNTexture))
    {
        LogAssert("cGraphicObjectFactory::createTargetTextureImp nvn texture register Failed");
    }

    Texture.setParam(Info);
	return TRUE;
}

//------------------------------------------------------------------------------
//! ターゲット・テクスチャの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createTargetTextureImp ( cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info )
{
    
    nvn::Format  RT_Format = GraphicUtilityImpl::GetColorFormat(Info.Format);
    nvn::TextureBuilder RenderTargetBuilder;
    RenderTargetBuilder.SetDefaults()
        .SetDevice(m_pDevice)
        .SetSize2D(Info.Width, Info.Height)
        .SetFlags(nvn::TextureFlags::COMPRESSIBLE)
        .SetFormat(RT_Format);
    if (Info.SamplingType> GD_MULTI_SAMPLING_TYPE_01)
    {
        RenderTargetBuilder.SetSamples(Info.SamplingType == GD_MULTI_SAMPLING_TYPE_02 ? 2 : 4);
    }

    uint32_t MipLevels = 1;
#if HW_MIPMAPPED_RT
    if (!Info.MipLevels) {
        MipLevels = 1 + (uint32_t)log2f(GET_MAXF(Info.Width, Info.Height));
    }
    else {
        MipLevels = Info.MipLevels;
    }
    MipLevels = GET_MIN(MipLevels, cRenderTargetImpl::NUM_SUBLEVELS_MAX + 1);

    Info.MipLevels = MipLevels;
#endif
    RenderTargetBuilder.SetLevels(MipLevels);

    if (Info.Dimesion == TEXTURE_DIMENSION_CUBEMAP)
    {
        RenderTargetBuilder.SetTarget(nvn::TextureTarget::TARGET_CUBEMAP);
    }
    else
    {
        RenderTargetBuilder.SetTarget(Info.SamplingType > GD_MULTI_SAMPLING_TYPE_01 ? nvn::TextureTarget::TARGET_2D_MULTISAMPLE : nvn::TextureTarget::TARGET_2D);
    }
 
    size_t renderTargetSize = RenderTargetBuilder.GetStorageSize();
    size_t renderTargetAlignment = RenderTargetBuilder.GetStorageAlignment();
    HW_UNUSED_VAR(renderTargetAlignment);

    Texture.m_pNVNTexture = new(*m_pWorkHeap) nvn::Texture();
    HW_ASSERT(Texture.m_pNVNTexture != nullptr);

    if (!Texture.initNvnTexture(*m_pGraphicMemoryAllocator, nullptr, renderTargetSize, GRAPHIC_MEMORY_RESOURCE_USAGE::COLOR_TARGET, RenderTargetBuilder))
    {
        LogAssert("cGraphicObjectFactory::createTargetTextureImp nvn texture init failed!");
    }
    if (!m_GraphicDescriptors.registerSampledTexture(&Texture.getSampledTextureDescriptor(), Texture.m_pNVNTexture))
    {
        LogAssert("cGraphicObjectFactory::createTargetTextureImp nvn texture register Failed");
    }

    return TRUE ;
}

//------------------------------------------------------------------------------
//! メモリ共有テクスチャ作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createShareTargetTexture ( cTextureImpl &Texture, const cTextureImpl &oBaseTextrue, const CREATE_TARGET_TEXTURE_INFO &Info )
{
    //Virtuos Todo: JH Shared base texture is using?
	NOT_IMPLEMENTED();

	return TRUE;
}

//------------------------------------------------------------------------------
//!	レンダーターゲットの作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createRenderTarget ( cRenderTargetImpl &Target, cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info )
{
    Target.m_pTexture = Texture.m_pNVNTexture;
	return TRUE;
}

//------------------------------------------------------------------------------
//!	ターゲット・テクスチャの作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createTargetTexture ( cRenderTargetImpl &Target, cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info )
{
    b32 Result;

    //Virtuos Todo: JH Shared base texture is using?
    //if (Info.pBaseTextrue != NULL) {
    //    Result = createShareTargetTexture(Texture, (*Info.pBaseTextrue), Info);
    //}
    //else
    {
        Result = createTargetTextureImp(Texture, Info);
    }

    if (!Result) {
        return FALSE;
    }

    if (!createRenderTarget(Target, Texture, Info)) {
        return FALSE;
    }

    Texture.setParam(Info);
    Target.setParam(Info);

    return TRUE;
}

//------------------------------------------------------------------------------
//!	Z値用・テクスチャの作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createZTexture ( cTextureImpl &Texture, const cDepthSurfaceImpl &Depth, u32 Width, u32 Height )
{
    HW_UNUSED_VAR(Width);
    HW_UNUSED_VAR(Height);

    nvn::Texture* DepthTexture = Depth.m_pInstance->m_pNVNTexture;
    nvn::TextureView& SampledTextureView = Texture.m_SampledTextureView;

    SampledTextureView.SetDefaults();
    SampledTextureView.SetFormat(DepthTexture->GetFormat());
    SampledTextureView.SetLevels(0, 1);
    SampledTextureView.SetTarget(DepthTexture->GetTarget());
    if (DepthTexture->GetDepth() == 6)
    {
        SampledTextureView.SetTarget(nvn::TextureTarget::TARGET_CUBEMAP);
        SampledTextureView.SetLayers(0, 6);
    }
    else
    {
        SampledTextureView.SetTarget(nvn::TextureTarget::TARGET_2D);
    }
    SampledTextureView.SetDepthStencilMode(nvn::TextureDepthStencilMode::DEPTH);

    // register descriptor 
    {
        if (!m_GraphicDescriptors.registerSampledTexture(&Texture.getSampledTextureDescriptor(), DepthTexture, &SampledTextureView))
        {
            return FALSE;
        }
    }

    Texture.setParam(Depth);
    Texture.m_pNVNTexture = DepthTexture;

	return TRUE;
}

//------------------------------------------------------------------------------
//!	空テクスチャの作成
//! @param[out]	rTexture	テクスチャ
//! @param[in]	rInfo		作成情報
//! @retval		TRUE		成功
//! @retval		FALSE		失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createEmptyTexture( cTextureImpl& rTexture , const CREATE_EMPTY_TEXTURE_INFO& rInfo )
{
	NOT_IMPLEMENTED();
	return TRUE;
}

//------------------------------------------------------------------------------
//!	テクスチャの作成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createTexture( cTextureImpl &Texture, const CREATE_TEXTURE_INFO &Info )
{
    Texture.m_pNVNTexture = new(*m_pWorkHeap) nvn::Texture();
    if (Texture.m_pNVNTexture == nullptr)
    {
        return FALSE;
    }

    //Virtuos JH: Dds texture is unsupported!
    u32 dwMagicNumber = *(const u32*)(Info.pData);
    HW_ASSERT(dwMagicNumber != HW_DDS_MAGIC);

    if (!createTextureFromXtx(Texture, Info))
    {
        return FALSE;
    }

    // register descriptor
    {
        if (!m_GraphicDescriptors.registerSampledTexture(&Texture.getSampledTextureDescriptor(), Texture.m_pNVNTexture))
        {
            return FALSE;
        }
    }


    return TRUE;
}

b32 cGraphicObjectFactory::createTextureFromXtx(cTextureImpl& Texture, const CREATE_TEXTURE_INFO& Info)
{
    TextureAsset* pTextureAsset = static_cast<TextureAsset*>(Info.pData);
    HW_ASSERT(pTextureAsset->header.id == TextureAsset::HeaderId);
    HW_ASSERT(Info.DataSize == (TextureAsset::HeaderLength + pTextureAsset->header.texelsDataSize));
   
    size_t TexelsDataSize = pTextureAsset->header.texelsDataSize;

    u32 Width = pTextureAsset->header.width;
    u32 Height = pTextureAsset->header.height;
    u32 Depth = pTextureAsset->header.depth;
    u32 MipLevels = pTextureAsset->header.numMipMapLevels;
    // create TextureBuilder based on TextureAsset header information
    nvn::TextureBuilder TextureBuilder;
    TextureBuilder.SetDefaults()
        .SetDevice(m_pDevice)
        .SetTarget(pTextureAsset->header.target)
        .SetFormat(pTextureAsset->header.format)
        .SetSize2D(Width, Height)
        .SetDepth(Depth)
        .SetLevels(MipLevels);

    void* SrcAddr = reinterpret_cast<char*>(pTextureAsset) + pTextureAsset->header.texelsOffset;

    if (!Texture.initNvnTexture(*m_pGraphicMemoryAllocator, SrcAddr, TexelsDataSize, GRAPHIC_MEMORY_RESOURCE_USAGE::SAMPLED_TEXTURE, TextureBuilder))
    {
        return FALSE;
    }

    TEXTURE_FORMAT Format = GraphicUtilityImpl::ConvertTextureFormat(pTextureAsset->header.format);
    Texture.setParam(Info, Width, Height, MipLevels, Format);

    return TRUE;
}

//------------------------------------------------------------------------------
//!	頂点バッファの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createVertexBuffer ( cVertexBufferImpl &Buffer, void * pData, const cVertexLayoutImpl &Layout, u32 StreamNo, u32 Num, cHeap *pHeap )
{
    HW_ASSERT(!Buffer.m_bRelease);
    HW_ASSERT(Buffer.m_pNVNBuffer == nullptr);
    u32 Stride = Layout.m_StreamStride[StreamNo];
    size_t BufferSize = (size_t)(Stride * Num);

    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::VERTEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);
    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);

    if (pData != nullptr)
    {
        memcpy(Buffer.m_pNVNBuffer->Map(), pData, BufferSize);
    }

    if (!Buffer.setBufferInfo(Layout, StreamNo, Num, FALSE))
    {
        return FALSE;
    }

    Buffer.m_bRelease = TRUE;
    Buffer.m_Num = Num;
    Buffer.m_Stride = Layout.m_StreamStride[StreamNo];

    Buffer.m_BufferAddress = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;
}

//------------------------------------------------------------------------------
//! 頂点バッファ作成
//! @param[out]	Buffer	頂点バッファ
//! @param[in]	Info	作成情報
//! @param[in]	pHeap	使用ヒープ
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createVertexBuffer ( cVertexBufferImpl &Buffer, const CREATE_VERTEX_BUFFER_INFO_IMPL&  Info , cHeap* pHeap )
{
    HW_ASSERT(!Buffer.m_bRelease);
    HW_ASSERT(Buffer.m_pNVNBuffer == nullptr);
    
    u32 Stride = Info.pLayout->m_StreamStride[Info.StreamNo];
    size_t BufferSize = (size_t)(Stride * Info.Num);

    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::VERTEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);

    if (Info.pData != nullptr)
    {
        memcpy(Buffer.m_pNVNBuffer->Map(), Info.pData, BufferSize);
    }

    if (!Buffer.setBufferInfo(*Info.pLayout, Info.StreamNo, Info.Num, FALSE))
    {
        return FALSE;
    }

    Buffer.m_bRelease    = TRUE;
    Buffer.m_Num         = Info.Num;
    Buffer.m_Stride      = Stride;
    Buffer.m_SRAType     = Info.SRAType;

    Buffer.m_BufferAddress = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;
}

//------------------------------------------------------------------------------
//!	頂点バッファ・ヒープの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createVertexBufferHeap ( cVertexBufferHeapImpl &BufferHeap, u32 BufferSize, cHeap &Heap )
{
    HW_UNUSE_ARG(Heap);
    
    BufferHeap.release();

    cVertexBufferImpl *pBuffer = new((*m_pWorkHeap))cVertexBufferImpl();
    if (!createDynamicVertexBuffer((*pBuffer), BufferSize)) 
    {
        delete (pBuffer);
        return FALSE;
    }

    BufferHeap.m_pBuffer = pBuffer;

    return TRUE;
}

b32 cGraphicObjectFactory::createDynamicVertexBuffer(cVertexBufferImpl& Buffer, u32 BufferSize)
{
    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::VERTEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);

    Buffer.m_bRelease    = TRUE;
    Buffer.m_Num         = 0;
    Buffer.m_Stride      = 0;

    Buffer.m_BufferAddress = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;

}

//------------------------------------------------------------------------------
//!	インデックスバッファの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createIndexBuffer ( cIndexBufferImpl &Buffer, void * pData, u32 Stride, u32 Num, cHeap *pHeap )
{
    size_t BufferSize = (size_t)(Stride * Num);

    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::INDEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);
    if (pData != nullptr)
    {
        memcpy(Buffer.m_pNVNBuffer->Map(), pData, BufferSize);
    }

    Buffer.m_bRelease = TRUE;
    Buffer.m_Num = Num;
    Buffer.m_Stride = Stride;

    Buffer.m_BufferAddress = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;
}

//------------------------------------------------------------------------------
//!	インデックスバッファの生成
//! @param[out]	Buffer	インデックスバッファ
//! @param[in]	Info	作成情報
//! @param[in]	pHeap	ヒープ
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createIndexBuffer ( cIndexBufferImpl &Buffer, const CREATE_INDEX_BUFFER_INFO& Info , cHeap* pHeap )
{
    size_t BufferSize = (size_t)(Info.Stride * Info.Num);

    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::INDEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);
    if (Info.pData != nullptr)
    {
        memcpy(Buffer.m_pNVNBuffer->Map(), Info.pData, BufferSize);
    }

    Buffer.m_bRelease       = TRUE;
    Buffer.m_Num            = Info.Num;
    Buffer.m_Stride         = Info.Stride;
    Buffer.m_SRAType        = Info.SRAType;

    Buffer.m_BufferAddress  = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;
}


//------------------------------------------------------------------------------
//!	インデックスバッファ・ヒープの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createIndexBufferHeap ( cIndexBufferHeapImpl &BufferHeap, u32 Stride, u32 Num, cHeap &Heap )
{
    HW_UNUSE_ARG(Heap);

    BufferHeap.release();

    cIndexBufferImpl * pBuffer = new((Heap))cIndexBufferImpl();

    if (!createDynamicIndexBuffer((*pBuffer), Stride, Num)) {
        delete (pBuffer);
        return FALSE;
    }
    
    BufferHeap.m_pBuffer = pBuffer;

    return TRUE;
}

b32 cGraphicObjectFactory::createDynamicIndexBuffer(cIndexBufferImpl &Buffer, u32 Stride, u32 Num)
{
    size_t BufferSize = (size_t)(Stride * Num);

    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::INDEX_BUFFER, BufferSize, HW_GRAPHIC_BUFFER_ALIGN, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);

    Buffer.m_bRelease       = TRUE;
    Buffer.m_Num            = Num;
    Buffer.m_Stride         = Stride;

    Buffer.m_BufferAddress  = Buffer.m_pNVNBuffer->GetAddress();

    return TRUE;
}
//------------------------------------------------------------------------------
//!	頂点レイアウトの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createVertexLayout ( cVertexLayoutImpl &Layout, VERTEX_LAYOUT_ELEMENT const *pElement, u32 ElementNum )
{
    static const u16 InvalidStreamIndex = 0xFF;

    u32 ValidElementNum = 0;
    for (u32 ElementIdx = 0; ElementIdx < ElementNum; ElementIdx++)
    {
        const VERTEX_LAYOUT_ELEMENT& oElement = pElement[ElementIdx];
        if (oElement.Stream == InvalidStreamIndex)
        {
            break;
        }

        ++ValidElementNum;
    }
    if (ValidElementNum == 0)
    {
        return FALSE;
    }

    Layout.m_pElement = new((*m_pWorkHeap))VERTEX_LAYOUT_ELEMENT[ValidElementNum];
    for (u32 ElementIdx = 0; ElementIdx < ValidElementNum; ElementIdx++)
    {
        const VERTEX_LAYOUT_ELEMENT& oElement = pElement[ElementIdx];
        memcpy(&Layout.m_pElement[ElementIdx], &oElement, sizeof(VERTEX_LAYOUT_ELEMENT));
    }

    u32 NumStreams = 0;
    for (u32 ElementIdx = 0; ElementIdx < ValidElementNum; ElementIdx++)
    {
        const VERTEX_LAYOUT_ELEMENT& oElement = pElement[ElementIdx];
        NumStreams = GET_MAX(NumStreams, oElement.Stream + 1u);
    }

    Layout.m_ElementNum = ValidElementNum;
    Layout.m_NumStreams = NumStreams;

    if (!Layout.setParam(Layout.m_pElement, ValidElementNum))
    {
        return FALSE;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//!	頂点シェーダーの生成
//! @param[out]	Shader	頂点シェーダー
//! @param[in]	rInfo	作成情報
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createVertexShader ( cVertexShaderImpl &Shader, const CREATE_VERTEX_SHADER_INFO& rInfo )
{
    Shader.load(rInfo.pData, *m_pDevice, nvn::ShaderStageBits::VERTEX, *m_pGraphicMemoryAllocator);

    return TRUE;
}

//------------------------------------------------------------------------------
//!	ピクセル・シェーダーの生成
//! @param[out]	Shader	ピクセルシェーダー
//! @param[int]	pData	シェーダーデータ
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createPixelShader ( cPixelShaderImpl &Shader, const void* pData )
{
    Shader.load(pData, *m_pDevice, nvn::ShaderStageBits::FRAGMENT, *m_pGraphicMemoryAllocator);
	
	return TRUE;
}

//------------------------------------------------------------------------------
//! コンピュートシェーダーの生成
//! @param[out]	Shader	シェーダー
//! @param[in]	pData	データ
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createComputeShader ( cComputeShaderImpl &Shader, const void * pData )
{
    Shader.load(pData, *m_pDevice, nvn::ShaderStageBits::COMPUTE, *m_pGraphicMemoryAllocator);

    return TRUE;
}

//------------------------------------------------------------------------------
//!	定数バッファの生成
//! @param[out]	Buffer		定数バッファ
//! @param[in]	pData		元データ
//! @param[in]	DataSize	データサイズ
//! @retval		TRUE		成功
//! @retval		FALSE		失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createConstantBuffer ( cConstantBufferImpl &Buffer, void *pData, size_t DataSize, cHeap *pHeap )
{
    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::UNIFORM_BUFFER, DataSize, cGraphicDeviceImpl::UniformBufferAlignment, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), DataSize);
    memcpy(Buffer.m_pNVNBuffer->Map(), pData, DataSize);

    Buffer.m_bDynamic = FALSE;
    Buffer.m_BufferSize = DataSize;
    Buffer.m_RegistSize = DataSize;
    Buffer.m_BufferNum = 1;

    Buffer.m_bRelease = TRUE;

    // register descriptor
    {
        if (!m_GraphicDescriptors.registerUniformBuffer(&Buffer.getUniformBufferDescriptor(), Buffer.m_pNVNBuffer, 0, DataSize))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//!	動的定数バッファの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createConstantBufferDynamic ( cConstantBufferImpl &Buffer, size_t DataSize, s32 BufferNum )
{
    m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::UNIFORM_BUFFER, DataSize * BufferNum, cGraphicDeviceImpl::UniformBufferAlignment, &Buffer.m_Allocation);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), DataSize * BufferNum);

    Buffer.m_bDynamic = TRUE;
    Buffer.m_BufferSize = DataSize;
    Buffer.m_RegistSize = 0;
    Buffer.m_BufferNum = BufferNum;

    Buffer.m_bRelease = TRUE;
    Buffer.m_pBuffer = Buffer.m_pNVNBuffer->Map();
    Buffer.m_bAllocFlag = FALSE;


    // register descriptor
    {
        if (!m_GraphicDescriptors.registerUniformBuffer(&Buffer.getUniformBufferDescriptor(), Buffer.m_pNVNBuffer, 0, DataSize))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//! コンピュートバッファ作成
//! @param[out]	pBuffer	バッファ
//! @param[in]	pData	入力データ
//! @param[in]	pHeap	ヒープ
//! @param[in]	rInfo	作成情報
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//! @note		可変長バッファの場合、Gds退避用のAtomicCounterバッファが必要です。
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createComputeBuffer( cComputeBufferImpl& Buffer , void* pData , cHeap* pHeap , const CREATE_COMPUTE_BUFFER_INFO& rInfo )
{
    HW_UNUSE_ARG(pHeap);

    size_t BufferSize = rInfo.Stride * rInfo.Num;
    size_t SystemAlign = GraphicUtilityImpl::GetComputeBufferAlign(rInfo.Type);
    SystemAlign = SystemAlign > rInfo.Alignment ? SystemAlign : rInfo.Alignment;
    b32 isVariableBuffer = GraphicUtility::isVariableComputeBuffer(rInfo.UsageFlag);
    size_t Alignment = rInfo.Alignment;
    HW_UNUSED_VAR(Alignment);

    auto ret = m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_BUFFER, BufferSize, SystemAlign, &Buffer.m_Allocation);
    HW_ASSERT(ret == GRAPHIC_MEMORY_ERROR::NONE);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), BufferSize);

    if (pData)
    {
        memcpy(Buffer.m_pNVNBuffer->Map(), pData, BufferSize);
    }
    else
    {
        memset(Buffer.m_pNVNBuffer->Map(), 0, BufferSize);
    }

    Buffer.m_bAlloc = TRUE;

    // When using as a variable length buffer, prepare a buffer for saving GDS.
    if (isVariableBuffer)
    {
        if (rInfo.pGdsCounterHeap == nullptr)
        {
            return FALSE;
        }

        Buffer.m_pGdsAtomicCounter = rInfo.pGdsCounterHeap->alloc(HW_GDS_ATOMIC_COUNTER_SIZE, HW_GDS_ATOMIC_COUNTER_ALIGN);
        if (!Buffer.m_pGdsAtomicCounter)
        {
            HW_ASSERT(false);
            return FALSE;
        }

        m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::STORAGE_BUFFER, HW_GDS_ATOMIC_COUNTER_SIZE, SystemAlign, &Buffer.m_AtomicCounterAllocation);
        Buffer.m_pAtomicCounterBuffer = createBuffer(Buffer.m_AtomicCounterAllocation.getNVNMemoryPool(), Buffer.m_AtomicCounterAllocation.getOffset(), HW_GDS_ATOMIC_COUNTER_SIZE);
        memset(Buffer.m_pAtomicCounterBuffer->Map(), 0, HW_GDS_ATOMIC_COUNTER_SIZE);
        
        if (!m_GraphicDescriptors.registerStorageBuffer(&Buffer.m_AtomicCounterDescriptor, Buffer.m_pAtomicCounterBuffer, 0, HW_GDS_ATOMIC_COUNTER_SIZE))
        {
            HW_ASSERT(false);
            return FALSE;
        }
    }

    Buffer.setParam(rInfo);

    // Register descriptor
    {
        if (!m_GraphicDescriptors.registerStorageBuffer(&Buffer.m_UnorderedAccess, Buffer.m_pNVNBuffer, 0, BufferSize))
        {
            HW_ASSERT(false);
            return FALSE;
        }
    }

    return TRUE;
}


//------------------------------------------------------------------------------
//! Indirect引数バッファの作成
//! @param[out]	Buffer	間接引数バッファ
//! @param[in]	Heap	ヒープ
//! @param[in]	rInfo	作成情報
//! @retval		TRUE	成功
//! @retval		FALSE	失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createIndirectArgsBuffer( cIndirectArgsBufferImpl& Buffer , cHeap& Heap , const CREATE_INDIRECTARGSBUFFER_INFO& rInfo )
{
    size_t DataSize = GraphicUtility::GetIndirectArgsBufferSize(rInfo.BufferType);

    GRAPHIC_MEMORY_ERROR error = m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::INDIRECT_BUFFER, DataSize, HW_INDIRECT_ARGS_BUFFER_ALIGN, &Buffer.m_Allocation);
    HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);

    Buffer.m_pNVNBuffer = createBuffer(Buffer.m_Allocation.getNVNMemoryPool(), Buffer.m_Allocation.getOffset(), DataSize);
    Buffer.m_bAlloc = true;

    if (rInfo.pData != nullptr)
    {
        void* pData = Buffer.m_pNVNBuffer->Map();
        memcpy(pData, rInfo.pData, DataSize);
    }
    else
    {
        memset(Buffer.m_pNVNBuffer->Map(), 0, DataSize);
    }

    Buffer.m_SRAType = rInfo.SRAType;
    Buffer.m_Type = rInfo.BufferType;
    Buffer.m_CpuAccessType = rInfo.CpuAccessType;


    if (rInfo.SRAType == SRA_TYPE_READ || rInfo.SRAType == SRA_TYPE_READWRITE)
    {
        if (!m_GraphicDescriptors.registerUniformBuffer(&Buffer.m_ShaderResource, Buffer.m_pNVNBuffer, 0, DataSize))
        {
            return FALSE;
        }
    }

    if (rInfo.SRAType == SRA_TYPE_WRITE || rInfo.SRAType == SRA_TYPE_READWRITE)
    {
        if (!m_GraphicDescriptors.registerStorageBuffer(&Buffer.m_UnorderedAccess, Buffer.m_pNVNBuffer, 0, DataSize))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//!	GPUラベル作成
//! @param[out]	oGpuLabel
//! @param[in]	Heap
//! @retval		TRUE
//! @retval		FALSE
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createGpuLabel( cGpuLabelImpl& oGpuLabel , cHeap& Heap )
{
	NOT_IMPLEMENTED();
	return TRUE;
}

//------------------------------------------------------------------------------
//!	ブレンド・ステートの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createBlendState ( cBlendStateImpl &State, const BLEND_STATE_DESC &Desc )
{
    nvn::ColorState& colorState = State.m_ColorState;
    colorState.SetDefaults();
    nvn::ChannelMaskState& channelMaskState = State.m_ChannelMaskState;
    channelMaskState.SetDefaults();

    for (int i = 0; i < RENDER_TARGET_MAX; ++i)
    {
         const u32 ColorWriteMask = Desc.ColorWriteMask[i];
         nvn::BlendState& blendState = State.m_BlendStates[i];
         blendState.SetBlendTarget(i);
         blendState.SetBlendFunc(
             GraphicUtilityImpl::ConvertBlendMode(Desc.SrcBlend),
             GraphicUtilityImpl::ConvertBlendMode(Desc.DestBlend),
             GraphicUtilityImpl::ConvertBlendMode(Desc.SrcBlendAlpha),
             GraphicUtilityImpl::ConvertBlendMode(Desc.DestBlendAlpha)
         );
         blendState.SetBlendEquation(
             GraphicUtilityImpl::ConvertBlendOp(Desc.BlendOp),
             GraphicUtilityImpl::ConvertBlendOp(Desc.BlendOpAlpha)
         );

         if ((Desc.SrcBlend != GS_BLEND_MODE_ONE)
             || (Desc.DestBlend != GS_BLEND_MODE_ZERO)
             || (Desc.BlendOp != GS_BLEND_OP_ADD)
             || (Desc.SrcBlendAlpha != GS_BLEND_MODE_ONE)
             || (Desc.DestBlendAlpha != GS_BLEND_MODE_ZERO)
             || (Desc.BlendOpAlpha != GS_BLEND_OP_ADD))
         {
             colorState.SetBlendEnable(i, true);
         }

         channelMaskState.SetChannelMask(i,
             ((ColorWriteMask & GS_COLOR_WRITE_ENABLE_R) == GS_COLOR_WRITE_ENABLE_R) ? NVN_TRUE : NVN_FALSE,
             ((ColorWriteMask & GS_COLOR_WRITE_ENABLE_G) == GS_COLOR_WRITE_ENABLE_G) ? NVN_TRUE : NVN_FALSE,
             ((ColorWriteMask & GS_COLOR_WRITE_ENABLE_B) == GS_COLOR_WRITE_ENABLE_B) ? NVN_TRUE : NVN_FALSE,
             ((ColorWriteMask & GS_COLOR_WRITE_ENABLE_A) == GS_COLOR_WRITE_ENABLE_A) ? NVN_TRUE : NVN_FALSE);
    }

    if (Desc.AlphaTestEnable)
    {
        colorState.SetAlphaTest(GraphicUtilityImpl::ConvertAlphaFunc(Desc.AlphaFunc));
    }
   
	return TRUE;
}

//------------------------------------------------------------------------------
//!	ラスタライズ・ステートの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createRasterizeState ( cRasterizeStateImpl &State, const RASTERIZE_STATE_DESC &Desc )
{
    nvn::MultisampleState& MultisampleState = State.m_MultisampleState;
    nvn::PolygonState& PolygonState = State.m_PolygonState;

    if (Desc.Wireframe)
    {
        PolygonState.SetPolygonMode(nvn::PolygonMode::LINE);
    }
    else
    {
        PolygonState.SetPolygonMode(nvn::PolygonMode::FILL);
    }

    PolygonState.SetCullFace(GraphicUtilityImpl::ConvertCullMode(Desc.CullMode));
    PolygonState.SetFrontFace(nvn::FrontFace::CW);
    PolygonState.SetPolygonOffsetEnables(Desc.DepthBiasEnable ? nvn::PolygonOffsetEnable::POINT |
        nvn::PolygonOffsetEnable::LINE | nvn::PolygonOffsetEnable::FILL
        : nvn::PolygonOffsetEnable::NONE);
    if (Desc.DepthBiasEnable)
    {
        State.m_DepthBias = (Desc.DepthBias * 16.0f);
        State.m_DepthBiasClamp = Desc.DepthBiasClamp;
        State.m_DepthSlopeBias = Desc.DepthSlopeBias;
    }
    else
    {
        State.m_DepthBias = 0.0f;
        State.m_DepthBiasClamp = 0.0f;
        State.m_DepthSlopeBias = 0.0f;
    }

    State.m_DepthClipEnable = Desc.DepthClipEnable;

    MultisampleState.SetDefaults();
    MultisampleState.SetMultisampleEnable(NVN_FALSE);

	return TRUE;
}

//------------------------------------------------------------------------------
//!	深度ステンシル・ステートの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createDepthStencilState ( cDepthStencilStateImpl &State, const DEPTH_STENCIL_STATE_DESC &Desc )
{
    nvn::DepthStencilState& DepthStencilState = State.m_DepthStencilState;
    DepthStencilState.SetDefaults();
    DepthStencilState.SetDepthTestEnable(Desc.DepthEnable);
    DepthStencilState.SetDepthWriteEnable(Desc.DepthWriteEnable);
    DepthStencilState.SetDepthFunc(GraphicUtilityImpl::ConvertDepthFunc(Desc.DepthFunc));
    State.m_DepthBoundsEnable = Desc.DepthBoundsEnable;

    switch (Desc.StencilMode)
    {
    case GS_STENCIL_MODE_NORMAL:
    case GS_STENCIL_MODE_TWO_SIDED:
        DepthStencilState.SetStencilTestEnable(NVN_TRUE);
        break;
    case GS_STENCIL_MODE_INVALID:
        DepthStencilState.SetStencilTestEnable(NVN_FALSE);
        break;
    default:
        return FALSE;
    }

    DepthStencilState.SetStencilFunc(nvn::Face::FRONT, GraphicUtilityImpl::ConvertStencilFunc(Desc.FrontFaceOp.StencilFunc));
    DepthStencilState.SetStencilOp(nvn::Face::FRONT,
        GraphicUtilityImpl::ConvertStencilOp(Desc.FrontFaceOp.StencilFailOp),
        GraphicUtilityImpl::ConvertStencilOp(Desc.FrontFaceOp.DepthFailOp),
        GraphicUtilityImpl::ConvertStencilOp(Desc.FrontFaceOp.StencilPassOp)
        );

    DepthStencilState.SetStencilFunc(nvn::Face::BACK, GraphicUtilityImpl::ConvertStencilFunc(Desc.BackFaceOp.StencilFunc));
    DepthStencilState.SetStencilOp(nvn::Face::BACK,
        GraphicUtilityImpl::ConvertStencilOp(Desc.BackFaceOp.StencilFailOp),
        GraphicUtilityImpl::ConvertStencilOp(Desc.BackFaceOp.DepthFailOp),
        GraphicUtilityImpl::ConvertStencilOp(Desc.BackFaceOp.StencilPassOp)
    );

    State.m_StencilRef = Desc.StencilRef;
    State.m_StencilReadMask = Desc.StencilReadMask;
    State.m_StencilWriteMask = Desc.DepthWriteEnable;
   
	return TRUE;
}

//------------------------------------------------------------------------------
//!	サンプラ・ステートの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createSamplerState ( cSamplerStateImpl &State, const SAMPLER_STATE_DESC &Desc )
{
    // create SamplerBuilder based on SAMPLER_STATE_DESC information
    nvn::SamplerBuilder SamplerBuilder;
    SamplerBuilder.SetDefaults();

    nvn::WrapMode Wrap[3];
    nvn::MinFilter MinFilter;
    nvn::MagFilter MagFilter;


    if (Desc.BorderColorIndex < 0) {
        static float const BorderColorTransBlack[4] = { 0.0f,0.0f,0.0f,0.0f };
        SamplerBuilder.SetBorderColor(BorderColorTransBlack);
    }
    else {
        if (Desc.BorderColorIndex >= m_BorderColorNum) return FALSE;

        SamplerBuilder.SetBorderColor(m_pBorderColor+ Desc.BorderColorIndex*4);
    }

    Wrap[0] = GraphicUtilityImpl::ConvertTextureAddress(Desc.AddressU);
    Wrap[1] = GraphicUtilityImpl::ConvertTextureAddress(Desc.AddressV);
    Wrap[2] = GraphicUtilityImpl::ConvertTextureAddress(Desc.AddressW);

    SamplerBuilder.SetWrapMode(Wrap[0], Wrap[1], Wrap[2]);

    MinFilter = GraphicUtilityImpl::ConverterTextureMinFilter(Desc.Filter, Desc.MipFilter);
    MagFilter = GraphicUtilityImpl::converterTextureMagFilter(Desc.Filter, Desc.MipFilter);

    SamplerBuilder.SetMinMagFilter(MinFilter, MagFilter);

    if (Desc.SetZFilter)
    {
        nvn::CompareFunc DepthFunc = GraphicUtilityImpl::converterTextureDepthFunc(Desc.DepthFunc);
        SamplerBuilder.SetCompare(nvn::CompareMode::COMPARE_R_TO_TEXTURE, DepthFunc);
    }

    SamplerBuilder.SetMaxAnisotropy(GraphicUtilityImpl::ConvertAnisotropyLevel(Desc.AnisotropyLevel));

    SamplerBuilder.SetDevice(m_pDevice);

    if (!State.initNvnSampler(&SamplerBuilder))
    {
        return FALSE;
    }

    // register descriptor
    {
        if (!m_GraphicDescriptors.registerSampler(&State.getSamplerDescriptor(), &State.m_Sampler))
        {
            return FALSE;
        }
    }


    return TRUE;
} 

//-----------------------------------------------------------------------------
//!	オクルージョン・クエリーの生成
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createOcclusionQuery ( cOcclusionQueryImpl &Query, cHeap &Heap )
{
    UNUSED_VARIABLE(Heap);
    size_t DataSize = sizeof(nvn::CounterData);

    GRAPHIC_MEMORY_ERROR error = m_pGraphicMemoryAllocator->allocMemory(GRAPHIC_MEMORY_RESOURCE_USAGE::QUERY_BUFFER, DataSize, cGraphicDeviceImpl::CounterAlignment, &Query.m_Allocation);
    HW_ASSERT(error == GRAPHIC_MEMORY_ERROR::NONE);

    Query.m_pNVNBuffer = createBuffer(Query.m_Allocation.getNVNMemoryPool(), Query.m_Allocation.getOffset(), DataSize);
    Query.m_bAlloc = TRUE;
       
	return TRUE;
}

//------------------------------------------------------------------------------
//! 非同期コンピュートコマンドバッファ生成
//! @param[out]	rDispatchCommandBufferImpl	非同期コンピュートコマンドバッファ
//! @param[in]	rBufferHeap					バッファヒープ
//! @retval		TRUE						成功
//! @retval		FALSE						失敗
//------------------------------------------------------------------------------
b32 cGraphicObjectFactory::createDispatchCommandBuffer( cDisaptchCommandBufferImpl& rDispatchCommandBufferImpl , cHeap& rBufferHeap )
{
	NOT_IMPLEMENTED();
	return TRUE;
}

nvn::Buffer * cGraphicObjectFactory::createBuffer(nvn::MemoryPool *pMemoryPool, u32 Offset, u32 BufferSize)
{
    nvn::BufferBuilder BufferBuilder;
    BufferBuilder.SetDefaults();
    BufferBuilder.SetDevice(m_pDevice);
    BufferBuilder.SetStorage(pMemoryPool, Offset, BufferSize);
    nvn::Buffer* pNVNBuffer = new((*m_pWorkHeap))nvn::Buffer();
    if (!pNVNBuffer->Initialize(&BufferBuilder))
    {
        LogAssert("Failed to createBuffer buffer");
    }
    return pNVNBuffer;
}

void cGraphicObjectFactory::createMemoryPool(cHeap* pHeap, size_t BufferSize, nvn::MemoryPoolFlags MemoryPoolFlags, void*& pPoolBuffer, nvn::MemoryPool*& pMemoryPool)
{
    size_t AlignedBufferSize = GET_ALIGN(BufferSize, NVN_MEMORY_POOL_STORAGE_GRANULARITY);
    if (pMemoryPool==nullptr)
    {
        pMemoryPool = new((*m_pWorkHeap))nvn::MemoryPool();
    }
    pPoolBuffer = pHeap->alloc(AlignedBufferSize, NVN_MEMORY_POOL_STORAGE_ALIGNMENT);
    if (pPoolBuffer == nullptr)
    {
        assert(false && "Failed to alloc memory, this is not allowed.");
        return;
    }

    nvn::MemoryPoolBuilder MemoryPoolBuilder;
    MemoryPoolBuilder.SetDefaults();
    MemoryPoolBuilder.SetDevice(m_pDevice);
    MemoryPoolBuilder.SetFlags(MemoryPoolFlags);
    MemoryPoolBuilder.SetStorage(pPoolBuffer, AlignedBufferSize);
    if (!pMemoryPool->Initialize(&MemoryPoolBuilder))
    {
        assert(false && "Initialize vertex buffer memory pool failed");
    }
}
//******************************************************************************
//	End of File
//******************************************************************************
