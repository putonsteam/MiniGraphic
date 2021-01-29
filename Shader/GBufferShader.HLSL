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

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;

	// Dynamically look up the texture in the array.
	diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);

	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);

	// Vector from point being lit to eye. 
	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Finish texture projection and sample SSAO map.
	pin.SsaoPosH /= pin.SsaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;

	// Light terms.
	float4 ambient = ambientAccess * gAmbientLight*diffuseAlbedo;

	// Only the first light casts a shadow.
//float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
//shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);
float shadowFactor = CalcShadowFactor(pin.ShadowPosH);
const float shininess = 1.0f - roughness;
Material mat = { diffuseAlbedo, fresnelR0, shininess };
float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
	pin.NormalW, toEyeW, shadowFactor);

float4 litColor = ambient + directLight;

// Add in specular reflections.
float3 r = reflect(-toEyeW, pin.NormalW);
//float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
float3 fresnelFactor = SchlickFresnel(fresnelR0, pin.NormalW, r);
litColor.rgb += shininess * fresnelFactor;// *reflectionColor.rgb;
//litColor.rgb = shadowFactor
// Common convention to take alpha from diffuse albedo.
litColor.a = diffuseAlbedo.a;

return litColor;
}

PixelOut PS(VertexOut outa) : SV_Target
{
	PixelOut pixelOut;

	//diffuse
	pixelOut.diffuse = DiffuseTexture.Sample(SampleWrapLinear, outa.Tex);
	
	//worldPos
	pixelOut.worldPos = float4(outa.worldPos, 1.0);

	//worldNormal
	float3 worldNormal = normalize(outa.W_Normal);

	//specular
	pixelOut.specular = SpecularTexture.Sample(SampleWrapLinear, outa.Tex);

	return pixelOut;
}