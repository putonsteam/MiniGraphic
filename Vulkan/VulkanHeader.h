#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <assert.h>
#include <memory>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ASSERT( a ) do { VkResult res = a; assert(VK_SUCCESS==res); }while(0)
#define VK_SUCCEED( a ) ( VK_SUCCESS == a )
#define VK_RETURN_IF_FAILED( a ) do{ VkResult res = a; assert(VK_SUCCESS==res); if( !VK_SUCCEED(res) ) return false; }while(0)
#include <vulkan/vulkan.h>
#include "./public/linmath.h"

// TODO: reference additional headers your program requires here
