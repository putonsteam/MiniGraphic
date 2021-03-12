//******************************************************************************
//	@file	HwGraphicObjectFactory.h
//	@brief	グラフィック・オブジェクト・ファクトリー( PS4 )
//	@author	Tsuyoshi Odera.
//******************************************************************************
#ifndef _HW_GRAPHIC_OBJECT_FACTORY_H_
#define _HW_GRAPHIC_OBJECT_FACTORY_H_

#include "HwGraphicDescriptorPool.h"

namespace Hw {
	class cVertexShaderImpl;
	class cPixelShaderImpl;
	class cConstantBufferImpl;
	class cConstantBufferImpl;
	class cBlendStateImpl;
	class cOcclusionQueryImpl;
	class cGraphicDeviceImpl;
    class cGraphicMemoryAllocator;

	// グラフィック・オブジェクト・ファクトリー( PS4 )
	class cGraphicObjectFactory {
	public:
		cGraphicObjectFactory();

		~cGraphicObjectFactory();

		void release ( void );	// 開放	
		
		b32 create ( nvn::Device *pDevice, cGraphicMemoryAllocator* pGraphicMemoryAllocator, const cGraphicSetupInfo &SetupInfo );	// 生成

		b32 createClearBufferObject ( cClearBufferObjectImpl &oObject, const cClearBufferShaderInfo &ShaderInfo );	// クリアバッファ・オブジェクトの生成
		b32 createResolveObject ( cResolveObjectImpl &oObject, const cResolveShaderInfo &ShaderInfo );				// リゾルブ・オブジェクトの生成 

		b32 createDepthSurface ( cDepthSurfaceImpl &oSurface, const CREATE_DEPTH_SURFACE_INFO &Info );								// 深度サーフェイスの生成

		b32 createLockableTexture ( cTextureImpl &Texture, const CREATE_LOCKABLE_TEXTURE_INFO &Info );							// ロック可能・テクスチャの作成
		b32 createTargetTexture ( cRenderTargetImpl &Target, cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info );	// ターゲット・テクスチャの作成
		b32 createZTexture ( cTextureImpl &Texture, const cDepthSurfaceImpl &Depth, u32 Width, u32 Height );						// Z値用・テクスチャの作成

		b32 createEmptyTexture( cTextureImpl& rTexture , const CREATE_EMPTY_TEXTURE_INFO& rInfo );					// 空テクスチャの作成
		b32 createTexture ( cTextureImpl &Texture, const CREATE_TEXTURE_INFO &Info );								// テクスチャの作成

		b32 createVertexBuffer ( cVertexBufferImpl &Buffer, void * pData, const cVertexLayoutImpl &Layout, u32 StreamNo, u32 Num, cHeap *pHeap );	// 頂点バッファの生成		
		b32 createVertexBuffer ( cVertexBufferImpl &Buffer, const CREATE_VERTEX_BUFFER_INFO_IMPL&  Info , cHeap* pHeap );		// 頂点バッファの生成	

		b32 createVertexBufferHeap ( cVertexBufferHeapImpl &BufferHeap, u32 BufferSize, cHeap &Heap );			// 頂点バッファ・ヒープの生成
		b32 createDynamicVertexBuffer(cVertexBufferImpl& Buffer, u32 BufferSize);
		
		b32 createIndexBuffer ( cIndexBufferImpl &Buffer, void * pData, u32 Stride, u32 Num, cHeap *pHeap );		// インデックスバッファの生成
		b32 createIndexBuffer ( cIndexBufferImpl &Buffer, const CREATE_INDEX_BUFFER_INFO& Info , cHeap* pHeap );			// インデックス・バッファの生成

		b32 createIndexBufferHeap ( cIndexBufferHeapImpl &BufferHeap, u32 Stride, u32 Num, cHeap &Heap );			// インデックスバッファ・ヒープの生成

        b32 createDynamicIndexBuffer(cIndexBufferImpl &Buffer, u32 Stride, u32 Num);                                // 動的インデックス・バッファの生成

		b32 createVertexLayout ( cVertexLayoutImpl &Layout, VERTEX_LAYOUT_ELEMENT const *pElement, u32 ElementNum );	// 頂点レイアウトの生成

		b32 createVertexShader ( cVertexShaderImpl &Shader, const CREATE_VERTEX_SHADER_INFO& rInfo );			// 頂点シェーダーの生成
		b32 createPixelShader ( cPixelShaderImpl &Shader, const void * pData );									// ピクセル・シェーダーの生成
		b32 createComputeShader ( cComputeShaderImpl &Shader, const void * pData );								// コンピュートシェーダーの生成

		b32 createConstantBuffer ( cConstantBufferImpl &Buffer, void *pData, size_t DataSize, cHeap *pHeap );	// コンスタント・バッファの生成
		b32 createConstantBufferDynamic ( cConstantBufferImpl &Buffer, size_t DataSize, s32 BufferNum );		// 動的コンスタント・バッファの生成

		b32 createComputeBuffer( cComputeBufferImpl& Buffer , void* pData , cHeap* pHeap , const CREATE_COMPUTE_BUFFER_INFO& rInfo );	// コンピュートバッファの生成

		b32 createIndirectArgsBuffer( cIndirectArgsBufferImpl& Buffer , cHeap& Heap , const CREATE_INDIRECTARGSBUFFER_INFO& rInfo );	// 間接引数バッファの作成

		b32 createGpuLabel( cGpuLabelImpl& oGpuLabel , cHeap& Heap );

		b32 createBlendState ( cBlendStateImpl &State, const BLEND_STATE_DESC &Desc );							// ブレンド・ステートの生成
		b32 createRasterizeState ( cRasterizeStateImpl &State, const RASTERIZE_STATE_DESC &Desc );				// ラスタライズ・ステートの生成
		b32 createDepthStencilState ( cDepthStencilStateImpl &State, const DEPTH_STENCIL_STATE_DESC &Desc );		// 深度ステンシル・ステートの生成
		b32 createSamplerState ( cSamplerStateImpl &State, const SAMPLER_STATE_DESC &Desc );						// サンプラ・ステートの生成

		b32 createOcclusionQuery ( cOcclusionQueryImpl &Query, cHeap &Heap );								// オクルージョン・クエリーの生成	 

		b32 entryTileInfo ( s32 &TileIndex, u32 &Offset, u32 Pitch, u32 Height, u32 Comp );					// タイル情報のエントリー
		b32 entrySetTileOffset ( u32 *pSetAddr, s32 TileIndex, u32 Offset, cDepthSurfaceImpl * pDepth );	// タイル領域の書き換えにエントリー

		b32 createDispatchCommandBuffer( cDisaptchCommandBufferImpl& rDispatchCommandBufferImpl , cHeap& rBufferHeap );			// 非同期コンピュートコマンドバッファ

        nvn::Device* const GetDevice() { return m_pDevice; }

        cGraphicDescriptorPool* getGraphicDescriptors() { return &m_GraphicDescriptors; }

    private:

		void clear ( void );	// クリア

		void setClearBuffer_DepthState ( DEPTH_STENCIL_STATE_DESC &DepthDesc, b32 Enable );		// クリアバッファ・深度・ステートの設定
		void setClearBuffer_StencilState ( DEPTH_STENCIL_STATE_DESC &DepthDesc, b32 Enable );	// クリアバッファ・ステンシル・ステートの設定

		b32 createTargetTextureImp ( cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info );		// ターゲット・テクスチャの生成
		
		b32 createShareTargetTexture ( cTextureImpl &Texture, const cTextureImpl &oBaseTextrue, const CREATE_TARGET_TEXTURE_INFO &Info );		// メモリ共有テクスチャの作成

        b32 createRenderTarget ( cRenderTargetImpl &Target, cTextureImpl &Texture, const CREATE_TARGET_TEXTURE_INFO &Info );	// レンダーターゲットの作成　　
        
        nvn::Buffer * createBuffer(nvn::MemoryPool* pMemoryPool, u32 Offset, u32 BufferSize);

        // Virtuos Todo: CT follow function is temp code, will be deleted later
        void createMemoryPool(cHeap* pHeap, size_t BufferSize, nvn::MemoryPoolFlags MemoryPoolFlags, void*& pPoolBuffer, nvn::MemoryPool*& pMemoryPool);

        b32 createTextureFromXtx(cTextureImpl& Texture, const CREATE_TEXTURE_INFO& Info);
        
    private:
        float *         m_pBorderColor;									// ボーダー・カラー

        u32             m_BorderColorNum;	//!< ボーダー・カラー数
        nvn::Device*    m_pDevice;


        cHeap *         m_pWorkHeap;        //!< 作業用ヒープ( VirtualMemory )
        cHeap *         m_pBufferHeap;        //!< バッファ用ヒープ( Onion )
        cHeap *         m_pVramHeap;        //!< VRAM用ヒープ( Garlic R/W )

        cGraphicMemoryAllocator* m_pGraphicMemoryAllocator;

        cGraphicDescriptorPool m_GraphicDescriptors;
    };
}

#endif	// _HW_GRAPHIC_OBJECT_FACTORY_H_

//******************************************************************************
//	End of File
//******************************************************************************