#include "PipeRunner.h"
#include "BrainRigActor.h"
#include "ProceduralObjActor.h"
#include "EngineUtils.h"
#include "ObjMeshLoader.h"

#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFilemanager.h"
#include "Materials/MaterialInstanceDynamic.h"

APipeRunner::APipeRunner()
{
    PrimaryActorTick.bCanEverTick = true;

    PendingDownloads = 0;
    bPipelineCompleted = false;
    bMeshesSpawned = false;

    BrainActor = nullptr;
    TumorActor = nullptr;
    AxialActor = nullptr;
    CoronalActor = nullptr;
    SagittalActor = nullptr;

    // =============================================
    // AUTO-LOAD MATERIALS (WORKING PATHS)
    // =============================================

    // Brain: /Script/Engine.Material'/Game/Materials/M_BrainGray.M_BrainGray'
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BrainMatFinder(
        TEXT("Material'/Game/Materials/M_BrainGray.M_BrainGray'")
    );
    if (BrainMatFinder.Succeeded())
    {
        BrainMaterial = BrainMatFinder.Object;
        UE_LOG(LogTemp, Log, TEXT("Loaded BrainMaterial: M_BrainGray"));
    }
    else
    {
        BrainMaterial = nullptr;
        UE_LOG(LogTemp, Error, TEXT("FAILED to load BrainMaterial. Check M_BrainGray path."));
    }

    // Tumor: /Script/Engine.Material'/Game/Materials/M_TumorRed.M_TumorRed'
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TumorMatFinder(
        TEXT("Material'/Game/Materials/M_TumorRed.M_TumorRed'")
    );
    if (TumorMatFinder.Succeeded())
    {
        TumorMaterial = TumorMatFinder.Object;
        UE_LOG(LogTemp, Log, TEXT("Loaded TumorMaterial: M_TumorRed"));
    }
    else
    {
        TumorMaterial = nullptr;
        UE_LOG(LogTemp, Error, TEXT("FAILED to load TumorMaterial. Check M_TumorRed path."));
    }
}

void APipeRunner::BeginPlay()
{
    Super::BeginPlay();

    // Kick off the backend pipeline
    RunPipeline();
}

void APipeRunner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void APipeRunner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner::EndPlay – cleaning up meshes."));
    ClearMeshes();
    DeleteSavedMeshes();
}

//////////////////////////////////////////////////////////////////////////
// PIPELINE START
//////////////////////////////////////////////////////////////////////////

void APipeRunner::RunPipeline()
{
    TSharedRef<IHttpRequest> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(TEXT("http://127.0.0.1:8000/pipeline/run"));
    Req->SetVerb(TEXT("POST"));
    Req->OnProcessRequestComplete().BindUObject(this, &APipeRunner::OnPipelineResponse);
    Req->SetTimeout(180.0f);
    Req->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("PipeRunner: Pipeline request started."));
}

//////////////////////////////////////////////////////////////////////////
// PIPELINE RESPONSE
//////////////////////////////////////////////////////////////////////////

void APipeRunner::OnPipelineResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline request failed: invalid response object."));
        return;
    }

    const FString Body = Response->GetContentAsString();
    UE_LOG(LogTemp, Warning, TEXT("Pipeline Response: %s"), *Body);

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON parse failed."));
        return;
    }

    FString Status;
    Json->TryGetStringField(TEXT("status"), Status);
    if (!Status.Equals(TEXT("success"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline reported non-success status: %s"), *Status);
        return;
    }

    // Local save paths in /Saved
    BrainPath = FPaths::ProjectSavedDir() / TEXT("brain.obj");
    TumorPath = FPaths::ProjectSavedDir() / TEXT("tumor.obj");
    AxialBottomPath = FPaths::ProjectSavedDir() / TEXT("axial_bottom.obj");
    CoronalFrontPath = FPaths::ProjectSavedDir() / TEXT("coronal_front.obj");
    SagittalRightPath = FPaths::ProjectSavedDir() / TEXT("sagittal_right.obj");

    bPipelineCompleted = true;
    PendingDownloads = 0;

    auto StartDownload = [&](const TCHAR* JsonKey, const FString& SaveLoc)
        {
            FString Url;
            if (Json->TryGetStringField(JsonKey, Url))
            {
                DownloadMesh(Url, SaveLoc);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing '%s' field."), JsonKey);
            }
        };

    StartDownload(TEXT("brain_mesh"), BrainPath);
    StartDownload(TEXT("tumor_mesh"), TumorPath);
    StartDownload(TEXT("axial_bottom"), AxialBottomPath);
    StartDownload(TEXT("coronal_front"), CoronalFrontPath);
    StartDownload(TEXT("sagittal_right"), SagittalRightPath);
}

//////////////////////////////////////////////////////////////////////////
// MESH DOWNLOAD HANDLING
//////////////////////////////////////////////////////////////////////////

void APipeRunner::DownloadMesh(const FString& Url, const FString& SavePath)
{
    if (Url.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("DownloadMesh: empty URL for %s"), *SavePath);
        return;
    }

    PendingDownloads++;

    TSharedRef<IHttpRequest> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(Url);
    Req->SetVerb(TEXT("GET"));

    Req->OnProcessRequestComplete().BindUObject(
        this,
        &APipeRunner::OnMeshDownloaded,
        SavePath
    );

    Req->SetTimeout(120.0f);
    Req->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Downloading %s -> %s"), *Url, *SavePath);
}

void APipeRunner::OnMeshDownloaded(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful,
    FString SavePath)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed downloading %s"), *SavePath);
    }
    else
    {
        if (FFileHelper::SaveArrayToFile(Response->GetContent(), *SavePath))
        {
            UE_LOG(LogTemp, Warning, TEXT("Saved mesh file: %s"), *SavePath);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to save mesh file: %s"), *SavePath);
        }
    }

    PendingDownloads--;

    if (PendingDownloads <= 0 && bPipelineCompleted && !bMeshesSpawned)
    {
        bMeshesSpawned = true;
        UE_LOG(LogTemp, Warning, TEXT("All downloads complete – spawning meshes"));
        SpawnMeshesFromSaved();
    }
}

//////////////////////////////////////////////////////////////////////////
// SPAWN INTO WORLD
//////////////////////////////////////////////////////////////////////////

void APipeRunner::SpawnMeshesFromSaved()
{
    UE_LOG(LogTemp, Warning, TEXT("Spawning brain + slices into scene"));

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnMeshesFromSaved: No World!"));
        return;
    }

    // Find the BrainRigActor to parent everything under
    ABrainRigActor* Rig = nullptr;
    for (TActorIterator<ABrainRigActor> It(World); It; ++It)
    {
        Rig = *It;
        break;
    }

    if (!Rig)
    {
        UE_LOG(LogTemp, Error, TEXT("No BrainRigActor found!"));
        return;
    }

    auto SpawnMesh = [&](const FString& Path,
        AProceduralObjActor*& OutActor,
        const TCHAR* Label,
        UMaterialInterface* Material)
        {
            FLoadedObjMesh Mesh;

            if (!FObjMeshLoader::LoadFromFile(Path, Mesh))
            {
                UE_LOG(LogTemp, Error, TEXT("Mesh load failed: %s"), *Path);
                return;
            }

            const FTransform SpawnTransform = Rig->GetActorTransform();

            OutActor = World->SpawnActor<AProceduralObjActor>(
                AProceduralObjActor::StaticClass(),
                SpawnTransform
            );

            if (!OutActor)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn %s"), Label);
                return;
            }

            OutActor->SetActorLabel(Label);
            OutActor->AttachToActor(Rig, FAttachmentTransformRules::KeepWorldTransform);

            // Build mesh FIRST
            OutActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);

            // Then apply material
            if (Material && OutActor->ProcMesh)
            {
                OutActor->ProcMesh->SetMaterial(0, Material);
                OutActor->ProcMesh->MarkRenderStateDirty();
            }

            UE_LOG(LogTemp, Warning, TEXT("Spawned mesh: %s | Materials=%d"),
                Label,
                OutActor->ProcMesh ? OutActor->ProcMesh->GetNumMaterials() : -1);
        };

    // Brain
    SpawnMesh(BrainPath, BrainActor, TEXT("BrainMesh"), BrainMaterial);
    if (BrainActor)
    {
        Rig->BrainActor = BrainActor;
    }

    // Tumor
    SpawnMesh(TumorPath, TumorActor, TEXT("TumorMesh"), TumorMaterial);
    if (TumorActor)
    {
        Rig->TumorActor = TumorActor;
    }

    // Axial slice
    SpawnMesh(AxialBottomPath, AxialActor, TEXT("AxialSlice"), BrainMaterial);
    if (AxialActor)
    {
        Rig->AxialActor = AxialActor;
        AxialActor->SetActorHiddenInGame(true);
    }

    // Coronal slice
    SpawnMesh(CoronalFrontPath, CoronalActor, TEXT("CoronalSlice"), BrainMaterial);
    if (CoronalActor)
    {
        Rig->CoronalActor = CoronalActor;
        CoronalActor->SetActorHiddenInGame(true);
    }

    // Sagittal slice
    SpawnMesh(SagittalRightPath, SagittalActor, TEXT("SagittalSlice"), BrainMaterial);
    if (SagittalActor)
    {
        Rig->SagittalActor = SagittalActor;
        SagittalActor->SetActorHiddenInGame(true);
    }

    // =====================================================
    // Create dynamic material instances on the Rig
    // (guarded so we don't get index warnings)
    // =====================================================
    if (Rig->BrainActor && Rig->BrainActor->ProcMesh &&
        Rig->BrainActor->ProcMesh->GetNumMaterials() > 0)
    {
        Rig->BrainMID = Rig->BrainActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
        UE_LOG(LogTemp, Log, TEXT("BrainMID created in PipeRunner."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("BrainMID not created – no material slot 0."));
    }

    if (Rig->TumorActor && Rig->TumorActor->ProcMesh &&
        Rig->TumorActor->ProcMesh->GetNumMaterials() > 0)
    {
        Rig->TumorMID = Rig->TumorActor->ProcMesh->CreateAndSetMaterialInstanceDynamic(0);
        UE_LOG(LogTemp, Log, TEXT("TumorMID created in PipeRunner."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TumorMID not created – no material slot 0."));
    }

    UE_LOG(LogTemp, Warning, TEXT("Meshes spawned successfully."));
}

//////////////////////////////////////////////////////////////////////////
// CLEANUP
//////////////////////////////////////////////////////////////////////////

void APipeRunner::DeleteSavedMeshes()
{
    const FString SavedDir = FPaths::ProjectSavedDir();
    IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Deleting OBJ mesh files in %s"), *SavedDir);

    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *SavedDir, TEXT("obj"));

    for (const FString& FileName : FoundFiles)
    {
        const FString FullPath = SavedDir / FileName;

        if (FileManager.FileExists(*FullPath))
        {
            if (FileManager.DeleteFile(*FullPath))
            {
                UE_LOG(LogTemp, Warning, TEXT("Deleted mesh file: %s"), *FullPath);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("FAILED to delete: %s"), *FullPath);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: OBJ cleanup complete."));
}

void APipeRunner::ClearMeshes()
{
    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Clearing spawned mesh actors..."));

    if (IsValid(BrainActor))
    {
        BrainActor->Destroy();
        BrainActor = nullptr;
    }

    if (IsValid(TumorActor))
    {
        TumorActor->Destroy();
        TumorActor = nullptr;
    }

    if (IsValid(AxialActor))
    {
        AxialActor->Destroy();
        AxialActor = nullptr;
    }

    if (IsValid(CoronalActor))
    {
        CoronalActor->Destroy();
        CoronalActor = nullptr;
    }

    if (IsValid(SagittalActor))
    {
        SagittalActor->Destroy();
        SagittalActor = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Mesh actors cleared."));
}
