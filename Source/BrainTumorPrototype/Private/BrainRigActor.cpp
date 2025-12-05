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

    // Delay detection until after actors spawn
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        this, &ABrainRigActor::DetectExistingActors
    );
}

/**
 * AUTO-DETECT brain + tumor if they exist in the world
 * (fallback for when PipeRunner hasn't assigned references yet)
 */
void ABrainRigActor::DetectExistingActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("BrainRigActor: Detecting actors..."));

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
                    BrainMID = BrainActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
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
                    TumorMID = TumorActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
                break;
            }
        }
    }
}

// ======================================================================
//  PUBLIC CONTROLS
// ======================================================================

void ABrainRigActor::ToggleTumorVisibility(bool bVisible)
{
    if (!TumorActor || !TumorActor->ProcMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleTumorVisibility: TumorActor is NULL"));
        return;
    }

    TumorActor->ProcMesh->SetVisibility(bVisible);
    UE_LOG(LogTemp, Log, TEXT("Tumor visibility: %s"), bVisible ? TEXT("ON") : TEXT("OFF"));
}

void ABrainRigActor::ShowHalfBrainSlice(AProceduralObjActor* NewSlice)
{
    if (!NewSlice)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowHalfBrainSlice: Slice NULL"));
        return;
    }

    // Hide full brain
    if (BrainActor && BrainActor->ProcMesh)
        BrainActor->ProcMesh->SetVisibility(false);

    // Hide tumor
    if (TumorActor && TumorActor->ProcMesh)
        TumorActor->ProcMesh->SetVisibility(false);

    // Hide previously active slice
    if (ActiveSlice && ActiveSlice->ProcMesh)
        ActiveSlice->ProcMesh->SetVisibility(false);

    // Show selected slice
    if (NewSlice->ProcMesh)
        NewSlice->ProcMesh->SetVisibility(true);

    ActiveSlice = NewSlice;

    UE_LOG(LogTemp, Log, TEXT("Showing slice: %s"), *NewSlice->GetName());
}

void ABrainRigActor::ShowFullBrain()
{
    // Show brain + tumor
    if (BrainActor && BrainActor->ProcMesh)
        BrainActor->ProcMesh->SetVisibility(true);

    if (TumorActor && TumorActor->ProcMesh)
        TumorActor->ProcMesh->SetVisibility(true);

    // Hide current slice
    if (ActiveSlice && ActiveSlice->ProcMesh)
        ActiveSlice->ProcMesh->SetVisibility(false);

    ActiveSlice = nullptr;

    UE_LOG(LogTemp, Log, TEXT("Showing full brain again"));
}
