cbuffer cbNeverChanges : register(b0)
{
	float4x4 gView;
	float4x4 gProjection;
	float2 Dimensions;
	float FarClip;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D WorldPosTex    : register(t0);
Texture2D gNormalMap     : register(t1);
Texture2D DeferredTex    : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

#define MAX_STEPS 300
#define MAX_INTERSECT_DIST 0.04

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD0;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];

    // Quad covering screen in NDC space.
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

float2 NormalizedDeviceCoordToScreenCoord(float2 ndc)
{
	float2 screenCoord;
	screenCoord.x = Dimensions.x * (ndc.x + 1.0f) / 2.0f;
	screenCoord.y = Dimensions.y * (1.0f - ((ndc.y + 1.0f) / 2.0f));
	return screenCoord;
}

float4 PS(VertexOut pin) : SV_TARGET
{
	// Get viewspace normal and z-coord of this pixel.  
	float3 n = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);

	float3 p = WorldPosTex.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;

	float3 ref = normalize(reflect(p, n));
	ref = mul(float4(ref, 1.0f), gView).xyz;

	p = mul(float4(p, 1.0f), gView).xyz;

   float2 coord;
   float2 origin = pin.TexC * Dimensions;
   float t = 1;
   float4 reflRay = float4(ref, 1.0f);
   float4 reflColor = float4(0, 0, 0, 0);

   // Tracing code from http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
   // c - view space coordinate
   // p - screen space coordinate
   // k - perspective divide
   float4 v0 = float4(p, 1.0f);
   float4 v1 = v0 + reflRay * FarClip;

   float4 p0 = mul(v0, gProjection);
   float4 p1 = mul(v1, gProjection);

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
   //if (reflectivity > 0.0f)
   {
	   while (t < maxSteps)
	   {
		  coord = origin + traceDir * t;
		  if (coord.x >= Dimensions.x || coord.y >= Dimensions.y || coord.x < 0 || coord.y < 0) break;

		  float curDepth = (v0 + dV * t).z;
		  curDepth /= k0 + dK * t; // Reverse the perspective divide back to view space
		  float3 q = WorldPosTex.SampleLevel(gsamPointClamp, coord, 0.0f).xyz;
		  float storedDepth = mul(float4(p, 1.0f), gView).z;
		  if (curDepth > storedDepth && curDepth - storedDepth < MAX_INTERSECT_DIST)
		  {
			 reflColor = DeferredTex[coord];
			 break;
		  }
		  t++;
	   }
   }
   return reflColor;
}
 