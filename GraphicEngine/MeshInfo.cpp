#include "MeshInfo.h"
#include "GraphicEngine.h"
#include "GeometryGenerator.h"
#include "ResourceStruct.h"

void MeshInfo::LoadTextMesh(const char* file)
{
	ifstream fin(file);

	if (!fin)
	{
		MessageBox(0, L"file not found.", 0, 0);
		//return nullptr;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		vertices[i].TexC = { 0.0f, 0.0f };

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);

		// Generate a tangent vector so normal mapping works.  We aren't applying
		// a texture map to the skull, so we just need any tangent vector so that
		// the math works out to give us the original interpolated vertex normal.
// 		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
// 		if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
// 		{
// 			XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
// 			XMStoreFloat3(&vertices[i].TangentU, T);
// 		}
// 		else
// 		{
// 			up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
// 			XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
// 			XMStoreFloat3(&vertices[i].TangentU, T);
// 		}


		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	BoundingBox bounds;
	XMStoreFloat3(&bounds.Center, 0.5f*(vMin + vMax));
	XMStoreFloat3(&bounds.Extents, 0.5f*(vMax - vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	vector<int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(int32_t);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &VertexBufferCPU));
	CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &IndexBufferCPU));
	CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	VertexBufferGPU = GetEngine()->CreateDefaultBuffer(vertices.data(), vbByteSize, VertexBufferUploader);

	IndexBufferGPU = GetEngine()->CreateDefaultBuffer(indices.data(), ibByteSize, IndexBufferUploader);

	VertexByteStride = sizeof(Vertex);
	VertexBufferByteSize = vbByteSize;
	IndexFormat = DXGI_FORMAT_R32_UINT;
	IndexBufferByteSize = ibByteSize;

	IndexCount = (UINT)indices.size();
}

void MeshInfo::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	
	IndexCount = (UINT)sphere.Indices32.size();
	auto totalVertexCount = sphere.Vertices.size();
		
	vector<Vertex> vertices(totalVertexCount);

	for (size_t i = 0; i < totalVertexCount; ++i)
	{
		vertices[i].Pos = sphere.Vertices[i].Position;
		vertices[i].Normal = sphere.Vertices[i].Normal;
		vertices[i].TexC = sphere.Vertices[i].TexC;
	}
	vector<uint16_t> indices;

	indices.insert(indices.end(), begin(sphere.GetIndices16()), end(sphere.GetIndices16()));
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &VertexBufferCPU));
	CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &IndexBufferCPU));
	CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	VertexBufferGPU = GetEngine()->CreateDefaultBuffer(vertices.data(), vbByteSize, VertexBufferUploader);

	IndexBufferGPU = GetEngine()->CreateDefaultBuffer(indices.data(), ibByteSize, IndexBufferUploader);

	VertexByteStride = sizeof(Vertex);
	VertexBufferByteSize = vbByteSize;
	IndexFormat = DXGI_FORMAT_R16_UINT;
	IndexBufferByteSize = ibByteSize;
}