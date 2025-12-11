#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// These headers define FHttpRequestPtr / FHttpResponsePtr typedefs
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "PipeRunner.generated.h"

class AProceduralObjActor;
class UMaterialInterface;

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
    UMaterialInterface* BrainMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* TumorMaterial = nullptr;

    // =====================================================
    //  BRAIN POSITION (OPTIONAL OFFSET / ROTATION)
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
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // Pipeline -> Unreal
    void RunPipeline();
    void OnPipelineResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // Downloading mesh files
    void DownloadMesh(const FString& Url, const FString& SavePath);
    void OnMeshDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString SavePath);

    // Spawn into scene
    void SpawnMeshesFromSaved();

    bool bMeshesSpawned = false;

    int32 PendingDownloads = 0;
    bool bPipelineCompleted = false;

    // Local saved .obj paths
    FString BrainPath;
    FString TumorPath;
    FString AxialBottomPath;
    FString CoronalFrontPath;
    FString SagittalRightPath;
};
