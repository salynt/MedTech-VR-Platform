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

    static_assert(TIsSame<decltype(Normals)::ElementType, FVector>::Value, "Normals must be FVector");
    static_assert(TIsSame<decltype(UV0)::ElementType, FVector2D>::Value, "UV0 must be FVector2D");

    // --- Generate Normals + Tangents ---
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Vertices,      
        Triangles,     
        UV0,
        Normals,
        Tangents      
    );


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

    // Enable lighting & collision behavior
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->ContainsPhysicsTriMeshData(true);
}
