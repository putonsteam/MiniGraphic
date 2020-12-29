#pragma once
#include "framework.h"

class LoadMaterial
{
public:
	void AddMaterial(string name, int CBIndex, DirectX::XMFLOAT4& diffuse, DirectX::XMFLOAT3& fresnel, float rough);
	void LoadTexture(string name, const wchar_t file);

// 	struct MaterialConstants
// 	{
// 		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
// 		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
// 		float Roughness = 0.25f;
// 
// 		// Used in texture mapping.
// 		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
// 	};

	// Simple struct to represent a material for our demos.  A production 3D engine
	// would likely create a class hierarchy of Materials.

		struct Material
	{
		// Unique material name for lookup.
		std::string Name;

		// Index into constant buffer corresponding to this material.
		int MatCBIndex = -1;

		// Index into SRV heap for diffuse texture.
		int DiffuseSrvHeapIndex = -1;

		// Index into SRV heap for normal texture.
		int NormalSrvHeapIndex = -1;

		// Dirty flag indicating the material has changed and we need to update the constant buffer.
		// Because we have a material constant buffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify a material we should set 
		// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
		//int NumFramesDirty = gNumFrameResources;

		// Material constant buffer data used for shading.
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = .25f;
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
		int TexIndex;
	}

private:
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	LoadTexture TextureList;
};