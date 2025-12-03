#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

struct FLoadedObjMesh
{
    TArray<FVector> Vertices;
    TArray<int32>   Triangles;   // indices (multiple of 3)
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
};

/**
 * Minimal OBJ loader for marching-cubes output:
 * - supports v, vn, vt, f (tri/quad)
 */
class FObjMeshLoader
{
public:
    static bool LoadFromFile(const FString& FilePath, FLoadedObjMesh& OutMesh);
};