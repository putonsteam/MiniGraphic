
#include "XGeometry.h"
#include "XBuffer.h"

#include "XShader.h"
#include "XTexture.h"

#include "..\DXSampleHelper.h"

extern XResourceThread *g_pResourceThread;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::map<std::wstring, XGeometry*> XGeometryManager::m_mResource;
XGeometry::~XGeometry()
{
	XBuffer::DeleteBuffer(m_pBuffer);
}
XGeometry* XGeometryManager::CreateGeometry(LPCWSTR pName,UINT uVertexCount, UINT uVertexStride, UINT uIndexCount, UINT uIndexFormat, UINT8* pGeometryData)
{
	//
	XGeometry *pGeometry = GetResource(pName);
	if (pGeometry)
	{
		return pGeometry;
	}

	if (!uVertexCount)
	{
		return nullptr;
	}

	//
	UINT uIndexSize = 0;
	switch (uIndexFormat)
	{
	case DXGI_FORMAT_R32_UINT:
		uIndexSize = 4;
		break;
	}
	if (!uIndexSize)
	{
		return nullptr;
	}

	pGeometry = new XGeometry(pName);
	AddResource(pName, pGeometry);

	//
	UINT uBufferSize = uVertexCount * uVertexStride + uIndexCount * uIndexSize;
	pGeometry->m_pBuffer = XBuffer::CreateBuffer(EBUFFERTYPE_BLOCK, uBufferSize, pGeometryData);
	if (!pGeometry->m_pBuffer)
	{
		delete pGeometry;
		return nullptr;
	}

	// Initialize the vertex buffer view.
	pGeometry->m_uNumVertexs = uVertexCount;
	pGeometry->m_VertexBufferView.BufferLocation = pGeometry->m_pBuffer->GetGpuAddress();
	pGeometry->m_VertexBufferView.StrideInBytes = uVertexStride;
	pGeometry->m_VertexBufferView.SizeInBytes = uVertexCount * uVertexStride;

	//
	pGeometry->m_uNumIndices = uIndexCount;
	if (uIndexCount)
	{
		// Describe the index buffer view.
		pGeometry->m_IndexBufferView.BufferLocation = pGeometry->m_VertexBufferView.BufferLocation + uVertexCount * uVertexStride;
		pGeometry->m_IndexBufferView.Format = (DXGI_FORMAT)uIndexFormat;
		pGeometry->m_IndexBufferView.SizeInBytes = uIndexCount * uIndexSize;
	}

	//
/*
	struct VSInput
	{
		float position[3];
		float normal[3];
		float uv[2];
		float tangent[3];
	};
	VSInput *pInput = (VSInput*)pGeometryData;
*/
	//
	pGeometry->m_vMin = Vector3f(10000, 10000, 10000);
	pGeometry->m_vMax = Vector3f(-10000, -10000, -10000);
	for (unsigned int i = 0;i < uVertexCount;++i)
	{
		Vector3f *pPos = (Vector3f*)(pGeometryData + i*uVertexStride);

		if (pPos->x < pGeometry->m_vMin.x)
		{
			pGeometry->m_vMin.x = pPos->x;
		}
		if (pPos->y < pGeometry->m_vMin.y)
		{
			pGeometry->m_vMin.y = pPos->y;
		}
		if (pPos->z < pGeometry->m_vMin.z)
		{
			pGeometry->m_vMin.z = pPos->z;
		}

		if (pPos->x > pGeometry->m_vMax.x)
		{
			pGeometry->m_vMax.x = pPos->x;
		}
		if (pPos->y > pGeometry->m_vMax.y)
		{
			pGeometry->m_vMax.y = pPos->y;
		}
		if (pPos->z > pGeometry->m_vMax.z)
		{
			pGeometry->m_vMax.z = pPos->z;
		}
	}

	return pGeometry;
}

XGeometry *pFullScreenGeometry = nullptr;
XGeometry *pXZPlaneGeometry = nullptr;
XGeometry *pPointGeometry = nullptr;
void XGeometry::Clean()
{
	//
	XGeometryManager::DelResource(&pFullScreenGeometry);
	XGeometryManager::DelResource(&pXZPlaneGeometry);
	XGeometryManager::DelResource(&pXZPlaneGeometry);
}

//
void Render(ID3D12GraphicsCommandList *pCommandList, XGeometry *pGeometry, XGraphicShader *pShader, XTextureSet *pTexture)
{
	if (pTexture)
	{
		pCommandList->SetGraphicsRootDescriptorTable(GRDT_SRV_TEXTURE, pTexture->GetSRVGpuHandle());
	}

	//
	pCommandList->SetPipelineState(pShader->GetPipelineState());

	//
	pCommandList->IASetVertexBuffers(0, 1, pGeometry->GetVertexBufferView());
	if (pGeometry->GetNumIndices())
	{
		pCommandList->IASetIndexBuffer(pGeometry->GetIndexBufferView());
		pCommandList->DrawIndexedInstanced(pGeometry->GetNumIndices(), 1, 0, 0, 0);
	}
}

//
class FullScreenResource : public IResourceLoad
{
public:
	virtual void LoadFromFile()
	{
		//
		struct Vertex
		{
			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 color;
			DirectX::XMFLOAT2 uv;
		};
		Vertex triangleVertices[] =
		{
			{ { -1.00f,  1.00f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },
			{ { -1.00f, -1.00f, 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
			{ { 1.00f, -1.00f, 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },

			{ { -1.00f,  1.00f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },
			{ { 1.00f, -1.00f, 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
			{ { 1.00f,  1.00f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f } },
		};
		UINT uIndex[] = { 0,1,2,3,4,5 };

		UINT8 *pData = new UINT8[6 * sizeof(Vertex) + 6 * sizeof(UINT)];
		UINT8 *pVertexData = pData;
		memcpy(pVertexData, &triangleVertices[0], 6 * sizeof(Vertex));
		UINT8 *pIndexData = pData + 6 * sizeof(Vertex);
		memcpy(pIndexData, &uIndex[0], 6 * sizeof(UINT));

		XGeometry *pGeometry = XGeometryManager::CreateGeometry(L"FullScreenGeometry", 6, sizeof(Vertex), 6, DXGI_FORMAT_R32_UINT, pData);//dynamic_cast<Geometry*>(GetXEngine()->GetGeometryManager()->CreateGeometry(L"UIGeometry"));
		if (pGeometry)
		{
			pFullScreenGeometry = pGeometry;
		}
		delete[] pData;
	}
	virtual void PostLoad()
	{
		//pUIManager->IncreaseResourceComplate();
	}
	virtual bool IsNeedWaitForResource()
	{
		return true;
	}
};
void RenderFullScreen(ID3D12GraphicsCommandList *pCommandList, XGraphicShader *pShader, XTextureSet *pTexture = nullptr)
{
	if (!pFullScreenGeometry)
	{
		FullScreenResource *pResource = new FullScreenResource();
		g_pResourceThread->InsertResourceLoadTask(pResource);
	}

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Render(pCommandList, pFullScreenGeometry, pShader, pTexture);
}

//
class XZPlaneResource : public IResourceLoad
{
public:
	virtual void LoadFromFile()
	{
		//
		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT3 normal;
			DirectX::XMFLOAT2 uv;
			DirectX::XMFLOAT3 tangent;
		};
		Vertex triangleVertices[] =
		{
			{ { -1.0f,  0.0f, -1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
			{ { -1.0f,  0.0f,  1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f } },
			{ {  1.0f,  0.0f,  1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 0.0f } },

			{ { -1.0f,  0.0f, -1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
			{ {  1.0f,  0.0f,  1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 0.0f } },
			{ {  1.0f,  0.0f, -1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		};
		UINT uIndex[] = { 0,1,2,3,4,5 };

		UINT8 *pData = new UINT8[6 * sizeof(Vertex) + 6 * sizeof(UINT)];
		UINT8 *pVertexData = pData;
		memcpy(pVertexData, &triangleVertices[0], 6 * sizeof(Vertex));
		UINT8 *pIndexData = pData + 6 * sizeof(Vertex);
		memcpy(pIndexData, &uIndex[0], 6 * sizeof(UINT));

		XGeometry *pGeometry = XGeometryManager::CreateGeometry(L"XZPlaneGeometry", 6, sizeof(Vertex), 6, DXGI_FORMAT_R32_UINT, pData);//dynamic_cast<Geometry*>(GetXEngine()->GetGeometryManager()->CreateGeometry(L"UIGeometry"));
		if (pGeometry)
		{
			pXZPlaneGeometry = pGeometry;
		}
		delete[] pData;
	}
	virtual void PostLoad()
	{
		//pUIManager->IncreaseResourceComplate();
	}
	virtual bool IsNeedWaitForResource()
	{
		return true;
	}
};
void RenderXZPlane(ID3D12GraphicsCommandList *pCommandList, XGraphicShader *pShader, XTextureSet *pTexture = nullptr)
{
	if (!pXZPlaneGeometry)
	{
		XZPlaneResource *pResource = new XZPlaneResource();
		g_pResourceThread->InsertResourceLoadTask(pResource);
	}

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Render(pCommandList, pXZPlaneGeometry, pShader, pTexture);
}

//
class PointResource : public IResourceLoad
{
	UINT m_uViewPixel,m_uViewWorld;
public:
	PointResource(UINT uViewPixel,UINT uViewWorld) : m_uViewPixel(uViewPixel), m_uViewWorld(uViewWorld){};
	virtual void LoadFromFile()
	{
		float fScale = (float)m_uViewWorld / (float)m_uViewPixel;

		//
		struct Vertex
		{
			DirectX::XMFLOAT3 position;
		};

		UINT uCountPerFace = m_uViewPixel * m_uViewPixel;
		UINT uCount = uCountPerFace * m_uViewPixel;
		UINT8 *pData = new UINT8[uCount * sizeof(Vertex) + uCount * sizeof(UINT)];

		Vertex *pVertex = (Vertex*)pData;
		for (unsigned int i = 0;i < m_uViewPixel;++i)
			for (unsigned int j = 0;j < m_uViewPixel;++j)
				for (unsigned int k = 0;k < m_uViewPixel;++k)
				{
					pVertex[i * uCountPerFace + j * m_uViewPixel + k].position.x = -1.0f * m_uViewWorld / 2.0f + (0.5f + k)* fScale;
					pVertex[i * uCountPerFace + j * m_uViewPixel + k].position.y = -1.0f * m_uViewWorld / 2.0f + (0.5f + j)* fScale;
					pVertex[i * uCountPerFace + j * m_uViewPixel + k].position.z = -1.0f * m_uViewWorld / 2.0f + (0.5f + i)* fScale;
				}
		UINT *pIndex = (UINT*)(pData + uCount * sizeof(Vertex));
		for (unsigned int i = 0;i < uCount;++i)
		{
			pIndex[i] = i;
		}
		
		//
		XGeometry *pGeometry = XGeometryManager::CreateGeometry(L"PointGeometry", uCount, sizeof(Vertex), uCount, DXGI_FORMAT_R32_UINT, pData);//dynamic_cast<Geometry*>(GetXEngine()->GetGeometryManager()->CreateGeometry(L"UIGeometry"));
		if (pGeometry)
		{
			pPointGeometry = pGeometry;
		}
		delete[] pData;
	}
	virtual void PostLoad()
	{
		//pUIManager->IncreaseResourceComplate();
	}
	virtual bool IsNeedWaitForResource()
	{
		return true;
	}
};
void RenderPoint(ID3D12GraphicsCommandList *pCommandList, XGraphicShader *pShader, XTextureSet *pTexture = nullptr)
{
	if (!pPointGeometry)
	{
		PointResource *pResource = new PointResource(32,16);
		g_pResourceThread->InsertResourceLoadTask(pResource);
	}

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	Render(pCommandList, pPointGeometry, pShader, pTexture);
}