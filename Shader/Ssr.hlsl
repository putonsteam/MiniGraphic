struct VSInput
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct PSInput
{
	float4 position 	: SV_POSITION;
	float3 positionW	: positionW;		// World space position
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
SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);



//Texture2D WorldPosTex : register(t0);
//Texture2D g_texture2 : register(t2);
//TextureCube g_EnvironmentLight : register(t3);
//Texture2D g_texture5 : register(t5);
//SamplerState g_sampler : register(s0);

struct PsOutput
{
	float4 color 	: SV_TARGET;
	//float depth 	: SV_DEPTH;
};

#define MAX_STEPS 300
#define MAX_INTERSECT_DIST 0.04

PSInput VS(VSInput input)
{
	PSInput result;

	float3 position = input.PosL;
	position.y += 1.0f;
	result.position = mul(float4(position, 1.0f), mViewProj);

	result.positionW = position;

	return result;
}

float2 NormalizedDeviceCoordToScreenCoord(float2 ndc)
{
	float4 Dimensions = float4(800, 600, 0, 0);

	float2 screenCoord;
	screenCoord.x = Dimensions.x * (ndc.x + 1.0f) / 2.0f;
	screenCoord.y = Dimensions.y * (1.0f - ((ndc.y + 1.0f) / 2.0f));
	return screenCoord;
}

PsOutput PS(PSInput input)
{
	float4 Dimensions = float4(800, 600, 0, 0);

	PsOutput result;
	float FarClip = 100;
	
	float4 reflColor = float4(0, 0, 0, 0);
	float t = 1;
	//float reflectivity = material[origin].x;


	float3 p = input.positionW;//float3(0.0,-8.0f,60.0f);//

	float3 vToPoint = normalize(p - vEyePos.xyz);
	float3 reflRay = reflect(vToPoint, float3(0, 1, 0));

	float3 d = normalize(input.positionW - vEyePos.xyz);
	float3 refl = normalize(reflect(d, float3(0, 1.0f, 0)));
	float3 ScreenRefl = mul(float4(refl, 0), mView);

	float4 v0 = mul(float4(input.positionW, 1), mView);
	float4 v1 = v0 + float4(ScreenRefl, 0) * FarClip;

	float4 p0 = mul(v0, Projection);
	float4 p1 = mul(v1, Projection);

	float k0 = 1.0f / p0.w;
	float k1 = 1.0f / p1.w;

	// 
	p0 *= k0;
	p1 *= k1;

	float2 uv = p0.xy;
	uv += 1.0f;
	uv /= 2.0f;
	uv.y = 1.0f - uv.y;
	int2 origin = uv * Dimensions;
	int2 coord;
	p0.xy = NormalizedDeviceCoordToScreenCoord(p0.xy);
	p1.xy = NormalizedDeviceCoordToScreenCoord(p1.xy);

	v0 *= k0;
	v1 *= k1;

	float divisions = length(p1 - p0);
	float3 dV = (v1 - v0) / divisions;
	float dK = (k1 - k0) / divisions;
	float2 traceDir = (p1 - p0) / divisions;

	float maxSteps = min(MAX_STEPS, divisions);
	//if (reflectivity > 0.0f)
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
	result.color = renderTx[origin] * (1.0f - 0.5) + reflColor * 0.5;

	return result;




	//uint nNum = 40;
	//float fStep = 40.0 / nNum;

	////
	//float4 color = float4(0, 0, 0, 0);
	//float value = 1.0f;
	//for (uint i = 1; i < nNum; ++i)
	//{
	//	float3 vPointW = p + vReflection * i*fStep;

	//	float4 vPointP = mul(float4(vPointW, 1.0f), mViewProj);
	//	vPointP /= vPointP.w;

	//	float2 uv = vPointP.xy;
	//	uv += 1.0f;
	//	uv /= 2.0f;
	//	uv.y = 1.0f - uv.y;

	//	//float4 tex = g_texture2.Sample(g_sampler, uv);
	//	//if (tex.a != 0.0f)
	//	//{
	//		float depthR = mul(float4(vPointW, 1.0f), mView).z;
	//		if (depthR <= -60.0f)
	//			continue;

	//		float3 positionT = WorldPosTex.Sample(gsamLinearClamp, uv).xyz;
	//		float depthT = mul(float4(positionT, 1.0f), mView).z;

	//		if (depthT > depthR)
	//		{
	//			color = DeferredTex.Sample(gsamLinearClamp, uv);
	//			value = (float)i / (float)nNum;
	//			break;
	//		}
	//	//}

	//}

	////float4 environment = g_EnvironmentLight.SampleLevel(g_sampler, vReflection, 0);
	////color = lerp(color, environment, value);

	//result.color = float4(color.xyz, 1.0f);

	////
	////float4 position = mul(float4(input.positionW.xyz, 1.0f), mViewProj);
	////result.depth = position.z/position.w;

	//return result;


}
