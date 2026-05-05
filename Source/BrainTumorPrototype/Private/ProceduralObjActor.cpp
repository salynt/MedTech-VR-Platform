#include "ProceduralObjActor.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/World.h"

AProceduralObjActor::AProceduralObjActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
    SetRootComponent(ProcMesh);

    // Enable proper lighting
    ProcMesh->bUseAsyncCooking = false;
    ProcMesh->SetCastShadow(true);
}

void AProceduralObjActor::BuildFromLoadedMesh(
    const TArray<FVector>& Vertices,
    const TArray<int32>& Triangles
)
{
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> Colors;

    // --- Generate simple UVs ---
    UV0.SetNum(Vertices.Num());
    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        UV0[i] = FVector2D(
            Vertices[i].X * 0.01f,
            Vertices[i].Y * 0.01f
        );
    }

    static_assert(TIsSame<typename TArray<FVector>::ElementType, FVector>::Value, "Normals must be FVector");
    static_assert(TIsSame<typename TArray<FVector2D>::ElementType, FVector2D>::Value, "UV0 must be FVector2D");

    // --- Generate Normals + Tangents ---
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Vertices,
        Triangles,
        UV0,
        Normals,
        Tangents
    );

    // Create section 0
    ProcMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UV0,
        Colors,
        Tangents,
        true
    );

    // Make sure material slot 0 exists so SetMaterial / MID index 0 is always valid
    if (ProcMesh->GetNumMaterials() == 0)
    {
        ProcMesh->SetMaterial(0, nullptr);
    }

    // Enable lighting & collision behavior
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->ContainsPhysicsTriMeshData(true);
}
