// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <dxgi1_6.h>
#include <d3d12.h>
//#include <atlbase.h>
#include "d3dx12.h"
#include <stdint.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include "DXSampleHelper.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include <windows.h>
#include "DDSTextureLoader.h"
#include "MathHelper.h"
#include "Macro.h"
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
//#include "DeviceResources.h"