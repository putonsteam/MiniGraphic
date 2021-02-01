#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION0;
	float2 TexC    : TEXCOORD;
	float3 NormalW : NORMAL;
};

struct PixelOut
{
	float4 diffuse:SV_Target1;
	float4 worldPos:SV_Target2;
	float4 worldNormal:SV_Target3;
	float4 material:SV_Target4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;

	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	return vout;
}

PixelOut PS(VertexOut pin) : SV_Target
{
	PixelOut pixelOut;
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	uint diffuseTexIndex = matData.DiffuseMapIndex;

	pixelOut.material = float4(matData.FresnelR0, matData.Roughness);

	// Dynamically look up the texture in the array.
	pixelOut.diffuse = matData.DiffuseAlbedo * gDiffuseMap[diffuseTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);

	// Interpolating normal can unnormalize it, so renormalize it.
	pixelOut.worldNormal = float4(normalize(pin.NormalW), 1.0f);

	pixelOut.worldPos = float4(pin.PosW, 1.0f);

	return pixelOut;
}
