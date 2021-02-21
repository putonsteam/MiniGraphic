// Include common HLSL code.
#include "Common.hlsl"

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.TexC = gTexCoords[vid];

	// Quad covering screen in NDC space.
	vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float3 worldPos = WorldPosTex.Sample(gsamLinearClamp, pin.TexC).xyz;
	float3 worldNormal = WorldNormalTex.Sample(gsamLinearClamp, pin.TexC).xyz;
	float4 diffuse = DiffuseTex.Sample(gsamLinearClamp, pin.TexC);
	float4 material = materialTex.Sample(gsamLinearClamp, pin.TexC);

	// Fetch the material data.
	float3 fresnelR0 = material.xyz;
	float  roughness = material.w;

	// Vector from point being lit to eye. 
	float3 toEyeW = normalize(gEyePosW - worldPos);

    // Finish texture projection and sample SSAO map.
    //pin.SsaoPosH /= pin.SsaoPosH.w;
    float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.TexC, 0.0f).r;

    // Light terms.
	float4 ambient = gAmbientLight * diffuse * ambientAccess;

	float shadowFactor = CalcShadowFactor(mul(float4(worldPos, 1.0f), gShadowTransform));
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, fresnelR0, shininess };
	float4 directLight = ComputeLighting(gLights, mat, worldPos,
		worldNormal, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

	// Add in specular reflections.
	float3 r = reflect(-toEyeW, worldNormal);
	//float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, worldNormal, r);
	litColor.rgb += shininess * fresnelFactor;// *reflectionColor.rgb;

	// Common convention to take alpha from diffuse albedo.
	litColor.a = diffuse.a;

	return litColor;
}


