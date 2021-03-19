#pragma once
#include "framework.h"

class LoadMaterial
{
public:
	void AddMaterial(string name, int CBIndex, XMFLOAT4& diffuse, XMFLOAT3& fresnel, float rough);
	void SetDiffuseSrv(const wchar_t* file);
	void SetNormaSrv(const wchar_t* file);
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
		XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = .25f;
		float SsrAttr = 0.0f;
		XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

private:
	
};