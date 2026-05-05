#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrainRigActor.generated.h"

// Forward declarations
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

public:

    // ----------------------------
    // Full brain & tumor actors
    // ----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* BrainActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* TumorActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* TumorClass1Actor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* TumorClass2Actor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* TumorClass3Actor = nullptr;

    // ----------------------------
    // Slice actors
    // ----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* AxialActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* CoronalActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* SagittalActor = nullptr;

    // Active slice currently shown
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Brain Rig")
    AProceduralObjActor* ActiveSlice = nullptr;

    // Material Instances (optional)
    UPROPERTY()
    UMaterialInstanceDynamic* BrainMID = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* TumorMID = nullptr;

    // ----------------------------
    // Blueprint callable controls
    // ----------------------------

    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void RotateRight();

    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void RotateLeft();

    /** Toggle tumor visibility */
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ToggleTumorVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowAxialSlice();

    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowCoronalSlice();

    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowSagittalSlice();

    /** Show slice (axial/coronal/sagittal) */
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowHalfBrainSlice(AProceduralObjActor* NewSlice);

    /** Hide slice & show full brain */
    UFUNCTION(BlueprintCallable, Category = "Brain Controls")
    void ShowFullBrain(AProceduralObjActor* NewSlice);
};
