#pragma once
#include "framework.h"
#include "MeshInfo.h"

class LoadTextMesh
{
public:
	static MeshGeometry* Load(const char* file);
};