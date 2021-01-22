#pragma once
#include "framework.h"

struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float Pad0;
	//float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float Pad1;
	//float FalloffEnd = 10.0f;                           // point/spot light only
	//DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	//float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16
