#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// These headers define FHttpRequestPtr / FHttpResponsePtr typedefs
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "PipeRunner.generated.h"

class AProceduralObjActor;
class UMaterialInterface;

/**
 * APipeRunner
 * UE 5.4-safe HTTP runner that:
 *  - POSTs to /pipeline/run on BeginPlay
 *  - Downloads brain.obj and tumor.obj on success
 */
UCLASS()
class BRAINTUMORPROTOTYPE_API APipeRunner : public AActor
{
    GENERATED_BODY()

public:
    APipeRunner();

    // Assign these in editor (recommended):
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* BrainMaterial;

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* TumorMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain Position")
    FVector BrainDestination = FVector(0.f, 0.f, 120.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain Position")
    FRotator BrainRotation = FRotator::ZeroRotator;


protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DelatTime) override;

private:
    void RunPipeline();
    void OnPipelineResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void DownloadMesh(const FString& Url, const FString& SavePath);

    void SpawnMeshesFromSaved();

    AProceduralObjActor* BrainActor = nullptr;
    AProceduralObjActor* TumorActor = nullptr;
    AProceduralObjActor* AxialActor = nullptr;
    AProceduralObjActor* CoronallActor = nullptr;
    AProceduralObjActor* SagittalActor = nullptr;

    bool bMeshesSpawned = false;
    bool IsBackendReady();
    void CheckBackendHealth();

};
