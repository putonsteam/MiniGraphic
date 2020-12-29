#include "LoadMaterial.h"

void LoadMaterial::AddMaterial(string name, int CBIndex, DirectX::XMFLOAT4& diffuse, DirectX::XMFLOAT3& fresnel, float rough)
{
	Material material;
	material.Name = name;
	material.MatCBIndex = CBIndex;
	//skullMat->DiffuseSrvHeapIndex = 4;
	//material.NormalSrvHeapIndex = 5;
	material.DiffuseAlbedo = diffuse;
	material.FresnelR0 = fresnel;
	material.Roughness = rough;
	mMaterials[name] = material;
}


void LoadMaterial::LoadTexture(string name, const wchar_t file);
{
	mMaterials[name].DiffuseSrvHeapIndex = TextureList.Load(file);
}
