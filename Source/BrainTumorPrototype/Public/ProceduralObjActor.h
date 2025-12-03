#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralObjActor.generated.h"

UCLASS()
class BRAINTUMORPROTOTYPE_API AProceduralObjActor : public AActor
{
    GENERATED_BODY()

public:
    AProceduralObjActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* ProcMesh;

    UFUNCTION(BlueprintCallable)
    void BuildFromLoadedMesh(
        const TArray<FVector>& Vertices,
        const TArray<int32>& Triangles
    );
};
