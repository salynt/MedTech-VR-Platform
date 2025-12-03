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

    BrainActor = nullptr;
    TumorActor = nullptr;
    HalfBrainActor = nullptr;

    BrainMID = nullptr;
    TumorMID = nullptr;
    HalfBrainMID = nullptr;
}

void ABrainRigActor::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        this,
        &ABrainRigActor::DetectExistingActors
    );
}

ABrainRigActor* FindRig(UWorld* World)
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ABrainRigActor::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        return Cast<ABrainRigActor>(FoundActors[0]); // use first found
    }

    return nullptr;
}

void ABrainRigActor::DetectExistingActors()
{
    UE_LOG(LogTemp, Warning, TEXT("BrainRigActor: Detecting actors in level..."));

    UWorld* World = GetWorld();
    if (!World) return;

    // --- FIND BRAIN ---
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
                UE_LOG(LogTemp, Warning, TEXT("Found BrainActor: %s"), *Found->GetName());

                if (BrainActor->ProcMesh)
                    BrainMID = BrainActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);

                break;
            }
        }
    }


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
                UE_LOG(LogTemp, Warning, TEXT("Found TumorActor: %s"), *Found->GetName());



                if (TumorActor->ProcMesh)
                    TumorMID = TumorActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);

                break;
            }
        }
    }

    if (!HalfBrainActor)
    {
        for (TActorIterator<AProceduralObjActor> It(World); It; ++It)
        {
            AProceduralObjActor* Found = *It;
            if (!Found) continue;

            const FString Label = Found->GetActorLabel();

            if (Label.Contains(TEXT("axial"), ESearchCase::IgnoreCase) ||
                Label.Contains(TEXT("sagittal"), ESearchCase::IgnoreCase) ||
                Label.Contains(TEXT("coronal"), ESearchCase::IgnoreCase))
            {
                HalfBrainActor = Found;
                UE_LOG(LogTemp, Warning, TEXT("Found Half-Brain Slice: %s"), *Found->GetName());

                if (HalfBrainActor->ProcMesh)
                    HalfBrainMID = HalfBrainActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);

                break;
            }
        }
    }


}

void ABrainRigActor::ToggleTumorVisibility(bool bVisible)
{
    if (!TumorActor || !TumorActor->ProcMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleTumorVisibility: TumorActor is NULL"));
        return;
    }

    TumorActor->ProcMesh->SetVisibility(bVisible);

    UE_LOG(LogTemp, Warning,
        TEXT("Tumor visibility set to: %s"),
        bVisible ? TEXT("Visible") : TEXT("Hidden"));
}

void ABrainRigActor::ShowHalfBrainSlice(AProceduralObjActor* NewSlice)
{
    if (!NewSlice)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowHalfBrainSlice: Slice NULL"));
        return;
    }

    // Hide full brain + tumor
    if (BrainActor && BrainActor->ProcMesh)
        BrainActor->ProcMesh->SetVisibility(false);

    if (TumorActor && TumorActor->ProcMesh)
        TumorActor->ProcMesh->SetVisibility(false);

    // Hide existing slice
    if (HalfBrainActor && HalfBrainActor->ProcMesh)
        HalfBrainActor->ProcMesh->SetVisibility(false);

    // Switch to new slice
    HalfBrainActor = NewSlice;

    if (HalfBrainActor->ProcMesh)
        HalfBrainActor->ProcMesh->SetVisibility(true);

    UE_LOG(LogTemp, Warning, TEXT("Now showing slice: %s"), *NewSlice->GetName());
}

void ABrainRigActor::ShowFullBrain()
{
    if (BrainActor && BrainActor->ProcMesh)
        BrainActor->ProcMesh->SetVisibility(true);

    if (TumorActor && TumorActor->ProcMesh)
        TumorActor->ProcMesh->SetVisibility(true);

    if (HalfBrainActor && HalfBrainActor->ProcMesh)
        HalfBrainActor->ProcMesh->SetVisibility(false);

    UE_LOG(LogTemp, Warning, TEXT("Showing full brain again"));
}
