#include "BrainRigActor.h"
#include "ProceduralObjActor.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ABrainRigActor::ABrainRigActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABrainRigActor::BeginPlay()
{
    Super::BeginPlay();

    // Delay detection until after meshes have been spawned by PipeRunner
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            this, &ABrainRigActor::DetectExistingActors
        );
    }
}

/**
 * Fallback auto-detection in case PipeRunner didn't assign BrainActor / TumorActor.
 * Looks for actors whose labels contain "BrainMesh" / "TumorMesh".
 */
void ABrainRigActor::DetectExistingActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Log, TEXT("BrainRigActor: Detecting existing brain/tumor actors..."));

    // ---------------------------
    // Find the brain
    // ---------------------------
    if (!BrainActor)
    {
        for (TActorIterator<AProceduralObjActor> It(World); It; ++It)
        {
            AProceduralObjActor* Found = *It;
            if (!Found) continue;

            const FString Label = Found->GetActorLabel();
            if (Label.Contains(TEXT("BrainMesh"), ESearchCase::IgnoreCase))
            {
                BrainActor = Found;
                UE_LOG(LogTemp, Log, TEXT("Detected BrainActor: %s"), *Found->GetName());

                if (BrainActor->ProcMesh)
                {
                    BrainMID = BrainActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
                    UE_LOG(LogTemp, Log, TEXT("BrainMID created in DetectExistingActors."));
                }
                break;
            }
        }
    }

    // ---------------------------
    // Find the tumor
    // ---------------------------
    if (!TumorActor)
    {
        for (TActorIterator<AProceduralObjActor> It(World); It; ++It)
        {
            AProceduralObjActor* Found = *It;
            if (!Found) continue;

            const FString Label = Found->GetActorLabel();
            if (Label.Contains(TEXT("TumorMesh"), ESearchCase::IgnoreCase))
            {
                TumorActor = Found;
                UE_LOG(LogTemp, Log, TEXT("Detected TumorActor: %s"), *Found->GetName());

                if (TumorActor->ProcMesh)
                {
                    TumorMID = TumorActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
                    UE_LOG(LogTemp, Log, TEXT("TumorMID created in DetectExistingActors."));
                }
                break;
            }
        }
    }
}

// ======================================================================
//  PUBLIC CONTROLS
// ======================================================================

void ABrainRigActor::RotateLeft()
{
    FRotator Rot = GetActorRotation();
    Rot.Yaw -= 45.f;
    SetActorRotation(Rot);
}

void ABrainRigActor::RotateRight()
{
    FRotator Rot = GetActorRotation();
    Rot.Yaw += 45.f;
    SetActorRotation(Rot);
}

void ABrainRigActor::ToggleTumorVisibility(bool bVisible)
{
    if (!TumorActor || !TumorActor->ProcMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleTumorVisibility: TumorActor or ProcMesh is NULL"));
        return;
    }

    TumorActor->ProcMesh->SetVisibility(bVisible, true);
    UE_LOG(LogTemp, Log, TEXT("Tumor visibility: %s"), bVisible ? TEXT("ON") : TEXT("OFF"));
}

void ABrainRigActor::ShowAxialSlice()
{
    if (!AxialActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowAxialSlice: AxialActor is NULL"));
        return;
    }

    ShowHalfBrainSlice(AxialActor);
}

void ABrainRigActor::ShowCoronalSlice()
{
    if (!CoronalActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowCoronalSlice: CoronalActor is NULL"));
        return;
    }

    ShowHalfBrainSlice(CoronalActor);
}

void ABrainRigActor::ShowSagittalSlice()
{
    if (!SagittalActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowSagittalSlice: SagittalActor is NULL"));
        return;
    }

    ShowHalfBrainSlice(SagittalActor);
}

/**
 * Hide full brain + tumor and show only the given slice.
 */
void ABrainRigActor::ShowHalfBrainSlice(AProceduralObjActor* NewSlice)
{
    if (!NewSlice)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowHalfBrainSlice: NewSlice is NULL"));
        return;
    }

    // Hide full brain + tumor
    if (BrainActor)  BrainActor->SetActorHiddenInGame(true);
    if (TumorActor)  TumorActor->SetActorHiddenInGame(true);

    // Hide all slices first so the previous one isn't left visible
    if (AxialActor)    AxialActor->SetActorHiddenInGame(true);
    if (CoronalActor)  CoronalActor->SetActorHiddenInGame(true);
    if (SagittalActor) SagittalActor->SetActorHiddenInGame(true);

    // Finally, show the selected slice
    NewSlice->SetActorHiddenInGame(false);

    UE_LOG(LogTemp, Log, TEXT("ShowHalfBrainSlice: Showing slice %s"), *NewSlice->GetName());
}

/**
 * Restore the full brain + tumor and hide all slices.
 */
void ABrainRigActor::ShowFullBrain(AProceduralObjActor* /*NewSlice*/)
{
    // Show brain + tumor
    if (BrainActor)  BrainActor->SetActorHiddenInGame(false);
    if (TumorActor)  TumorActor->SetActorHiddenInGame(false);

    // Hide slices
    if (AxialActor)    AxialActor->SetActorHiddenInGame(true);
    if (CoronalActor)  CoronalActor->SetActorHiddenInGame(true);
    if (SagittalActor) SagittalActor->SetActorHiddenInGame(true);

    UE_LOG(LogTemp, Log, TEXT("ShowFullBrain: Restored brain + tumor, hid all slices."));
}
