struct PSInput
{
	float4 PosH : SV_POSITION;

	float2 TexC : TEXCOORD0;
};

cbuffer FrameBuffer : register(b0)
{
	float4x4 mView;
	float4x4 mViewProj;
	float4x4 Projection;
	float3   vEyePos;
};



// Nonnumeric values cannot be added to a cbuffer.
Texture2D camPos    : register(t0);
Texture2D gNormalMap     : register(t1);
Texture2D renderTx    : register(t2);
Texture2D FeatureAttr    : register(t3);
SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);



//Texture2D WorldPosTex : register(t0);
//Texture2D g_texture2 : register(t2);
//TextureCube g_EnvironmentLight : register(t3);
//Texture2D g_texture5 : register(t5);
//SamplerState g_sampler : register(s0);



static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

#define MAX_STEPS 300
#define MAX_INTERSECT_DIST 0.04

PSInput VS(uint vid : SV_VertexID)
{
	PSInput vout;

	vout.TexC = gTexCoords[vid];

	// Quad covering screen in NDC space.
	vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

	return vout;
}

float2 NormalizedDeviceCoordToScreenCoord(float2 ndc)
{
	float4 Dimensions = float4(800, 600, 0, 0);

	float2 screenCoord;
	screenCoord.x = Dimensions.x * (ndc.x + 1.0f) / 2.0f;
	screenCoord.y = Dimensions.y * (1.0f - ((ndc.y + 1.0f) / 2.0f));
	return screenCoord;
}

float4 PS(PSInput input) : SV_Target
{
	float4 Dimensions = float4(800, 600, 0, 0);

	float FarClip = 100;
	int2 coord;
	int2 origin = input.TexC * Dimensions;
	
	float4 reflColor = float4(0, 0, 0, 0);
	float t = 1;
	float reflectivity = FeatureAttr[origin].x;

	float3 p = camPos[origin].xyz;
	float3 d = normalize(p - vEyePos.xyz);
	float3 n = gNormalMap[origin].xyz;
	float3 refl = normalize(reflect(d, n));
	float3 ScreenRefl = mul(float4(refl, 0), mView);

	float4 v0 = mul(float4(p, 1), mView);
	float4 v1 = v0 + float4(ScreenRefl, 0) * FarClip;

	float4 p0 = mul(v0, Projection);
	float4 p1 = mul(v1, Projection);

	float k0 = 1.0f / p0.w;
	float k1 = 1.0f / p1.w;

	// 
	p0 *= k0;
	p1 *= k1;

	p0.xy = NormalizedDeviceCoordToScreenCoord(p0.xy);
	p1.xy = NormalizedDeviceCoordToScreenCoord(p1.xy);

	v0 *= k0;
	v1 *= k1;

	float divisions = length(p1 - p0);
	float3 dV = (v1 - v0) / divisions;
	float dK = (k1 - k0) / divisions;
	float2 traceDir = (p1 - p0) / divisions;

	float maxSteps = min(MAX_STEPS, divisions);
	if (reflectivity > 0.0f)
	{
		while (t < maxSteps)
		{
			coord = origin + traceDir * t;
			if (coord.x >= Dimensions.x || coord.y >= Dimensions.y || coord.x < 0 || coord.y < 0) break;

			float curDepth = (v0 + dV * t).z;
			curDepth /= k0 + dK * t; // Reverse the perspective divide back to view space
			float storedDepth = mul(float4(camPos[coord]), mView).z;
			if (curDepth > storedDepth && curDepth - storedDepth < MAX_INTERSECT_DIST)
			{
				reflColor = renderTx[coord];
				break;
			}
			t++;
		}
	}
	return renderTx[origin] * (1.0f - reflectivity) + reflColor * reflectivity;



}
