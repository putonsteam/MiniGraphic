#include "LoadMaterial.h"
#include "GraphicEngine.h"

void LoadMaterial::AddMaterial(string name, int CBIndex, XMFLOAT4& diffuse, XMFLOAT3& fresnel, float rough)
{
	Name = name;
	MatCBIndex = CBIndex;
	//skullMat->DiffuseSrvHeapIndex = 4;
	//NormalSrvHeapIndex = 5;
	DiffuseAlbedo = diffuse;
	FresnelR0 = fresnel;
	Roughness = rough;
	//mMaterials[name] = material;
}

void LoadMaterial::SetDiffuseSrv(const wchar_t* file)
{
	DiffuseSrvHeapIndex = GetEngine()->GetTextureList()->Load(file);
}

void LoadMaterial::SetNormaSrv(const wchar_t* file)
{
	NormalSrvHeapIndex = GetEngine()->GetTextureList()->Load(file);
}
