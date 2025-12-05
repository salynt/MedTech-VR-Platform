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


    // =====================================================
    //  MESH ACTOR REFERENCES (brain, tumor, slices)
    // =====================================================

    // Full brain
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
    AProceduralObjActor* BrainActor = nullptr;

    // Tumor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
    AProceduralObjActor* TumorActor = nullptr;

    // Slices
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
    AProceduralObjActor* AxialActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
    AProceduralObjActor* CoronalActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meshes")
    AProceduralObjActor* SagittalActor = nullptr;

    // =====================================================
    //  MATERIAL REFERENCES (OPTIONAL)
    // =====================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* BrainMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* TumorMaterial;

	// =====================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain Position")
    FVector BrainDestination = FVector(0.f, 0.f, 120.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain Position")
    FRotator BrainRotation = FRotator::ZeroRotator;

    UFUNCTION(BlueprintCallable)
    void ClearMeshes();

    UFUNCTION(BlueprintCallable)
    void DeleteSavedMeshes();


protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DelatTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


private:
    void RunPipeline();
    void OnPipelineResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void DownloadMesh(const FString& Url, const FString& SavePath);

    void SpawnMeshesFromSaved();

    bool bMeshesSpawned = false;
    bool IsBackendReady();
    void CheckBackendHealth();

};
