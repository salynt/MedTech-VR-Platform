#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrainRigActor.generated.h"

// Forward declaration (prevents include loops)
class AProceduralObjActor;
class UMaterialInstanceDynamic;

UCLASS()
class BRAINTUMORPROTOTYPE_API ABrainRigActor : public AActor
{
    GENERATED_BODY()

public:
    ABrainRigActor();

protected:
    virtual void BeginPlay() override;

private:


    void DetectExistingActors();

    void FindRigActor();

public:


    // Full brain mesh
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* BrainActor;

    // Tumor mesh
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* TumorActor;

    // Axial/Sagittal/Coronal slices (one active at a time)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* HalfBrainActor;


    UPROPERTY()
    UMaterialInstanceDynamic* BrainMID;

    UPROPERTY()
    UMaterialInstanceDynamic* TumorMID;

    UPROPERTY()
    UMaterialInstanceDynamic* HalfBrainMID;


    // Toggle tumor visibility ON/OFF
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ToggleTumorVisibility(bool bVisible);

    // Show a selected half-brain slice (axial, coronal, sagittal)
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowHalfBrainSlice(AProceduralObjActor* NewSlice);

    // Return to full brain (hide slice)
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowFullBrain();
};