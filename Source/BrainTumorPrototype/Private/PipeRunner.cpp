#include "PipeRunner.h"
#include "ProceduralObjActor.h"
#include "ObjMeshLoader.h"
#include "BrainRigActor.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h" // for TActorIterator

APipeRunner::APipeRunner()
{
    PrimaryActorTick.bCanEverTick = true;

    // Auto-load brain material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BrainMat(
        TEXT("Material'/Game/Materials/M_BrainGray.M_BrainGray'")
    );
    if (BrainMat.Succeeded())
    {
        BrainMaterial = BrainMat.Object;
    }

    // Auto-load tumor material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TumorMat(
        TEXT("Material'/Game/Materials/M_TumorRed.M_TumorRed'")
    );
    if (TumorMat.Succeeded())
    {
        TumorMaterial = TumorMat.Object;
    }
}

void APipeRunner::BeginPlay()
{
    Super::BeginPlay();
    RunPipeline();

    /*
    // Old backend-launch code omitted for clarity
    CheckBackendHealth();
    */
}

void APipeRunner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only spawn once
    if (!bMeshesSpawned)
    {
        const FString BrainPath = FPaths::ProjectSavedDir() / TEXT("brain.obj");
        const FString TumorPath = FPaths::ProjectSavedDir() / TEXT("tumor.obj");

        // Wait until both exist
        if (FPaths::FileExists(BrainPath) && FPaths::FileExists(TumorPath))
        {
            UE_LOG(LogTemp, Warning, TEXT("PipeRunner: OBJ files detected! Spawning actors..."));

            SpawnMeshesFromSaved();
            bMeshesSpawned = true;
        }
    }
}

void APipeRunner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner::EndPlay called — cleaning up meshes."));

    ClearMeshes();
    DeleteSavedMeshes();
}


void APipeRunner::RunPipeline()
{
    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Running FastAPI Pipeline..."));

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL(TEXT("http://127.0.0.1:8000/pipeline/run"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    Request->OnProcessRequestComplete().BindUObject(
        this, &APipeRunner::OnPipelineResponse
    );

    Request->ProcessRequest();
}

void APipeRunner::OnPipelineResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful)
{
    if (!Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline request failed: invalid response object."));
        return;
    }

    const int32 Code = Response->GetResponseCode();
    const FString Body = Response->GetContentAsString();

    UE_LOG(LogTemp, Warning, TEXT("Pipeline HTTP result: SuccessFlag=%s, Code=%d"),
        bWasSuccessful ? TEXT("true") : TEXT("false"),
        Code);
    UE_LOG(LogTemp, Warning, TEXT("Pipeline Response Body: %s"), *Body);

    if (Code < 200 || Code >= 300)
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline HTTP error: %d"), Code);
        return;
    }

    // Parse JSON
    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(Body);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("JSON parsing failed for pipeline response."));
        return;
    }

    // Handle status field from backend
    FString Status;
    if (JsonObject->TryGetStringField(TEXT("status"), Status))
    {
        if (!Status.Equals(TEXT("success"), ESearchCase::IgnoreCase))
        {
            FString Msg;
            JsonObject->TryGetStringField(TEXT("message"), Msg);
            UE_LOG(LogTemp, Error, TEXT("Pipeline reported error: %s"), *Msg);
            return;
        }
    }

    FString BrainUrl;
    FString TumorUrl;
    FString AxialTopUrl;
    FString AxialBottomUrl;
    FString CoronalFrontUrl;
    FString CoronalBackUrl;
    FString SagittalLeftUrl;
    FString SagittalRightUrl;

    if (!JsonObject->TryGetStringField(TEXT("brain_mesh"), BrainUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'brain_mesh' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("tumor_mesh"), TumorUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'tumor_mesh' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("axial_top"), AxialTopUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'axial_top' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("axial_bottom"), AxialBottomUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'axial_bottom' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("coronal_front"), CoronalFrontUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'coronal_front' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("coronal_back"), CoronalBackUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'coronal_back' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("sagittal_left"), SagittalLeftUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'sagittal_left' field."));
        return;
    }
    if (!JsonObject->TryGetStringField(TEXT("sagittal_right"), SagittalRightUrl))
    {
        UE_LOG(LogTemp, Error, TEXT("Pipeline JSON missing 'sagittal_right' field."));
        return;
    }

    const FString SaveBrain = FPaths::ProjectSavedDir() / TEXT("brain.obj");
    const FString SaveTumor = FPaths::ProjectSavedDir() / TEXT("tumor.obj");
    const FString SaveHalf_AxialBottom = FPaths::ProjectSavedDir() / TEXT("axial_bottom.obj");
    const FString SaveHalf_CoronalFront = FPaths::ProjectSavedDir() / TEXT("coronal_front.obj");
    const FString SaveHalf_SagittalRight = FPaths::ProjectSavedDir() / TEXT("sagittal_right.obj");

    DownloadMesh(BrainUrl, SaveBrain);
    DownloadMesh(TumorUrl, SaveTumor);
    DownloadMesh(AxialBottomUrl, SaveHalf_AxialBottom);
    DownloadMesh(CoronalFrontUrl, SaveHalf_CoronalFront);
    DownloadMesh(SagittalRightUrl, SaveHalf_SagittalRight);
}

void APipeRunner::DownloadMesh(const FString& Url, const FString& SavePath)
{
    if (Url.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Download skipped: empty URL"));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));

    Request->OnProcessRequestComplete().BindLambda(
        [SavePath](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bWasSuccessful)
        {
            if (bWasSuccessful && Resp.IsValid())
            {
                const bool bSaved =
                    FFileHelper::SaveArrayToFile(Resp->GetContent(), *SavePath);

                if (bSaved)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Downloaded: %s"), *SavePath);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to save: %s"), *SavePath);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to download: %s"), *SavePath);
            }
        }
    );

    Request->ProcessRequest();
}

void APipeRunner::SpawnMeshesFromSaved()
{
    const FString BrainPath = FPaths::ProjectSavedDir() / TEXT("brain.obj");
    const FString TumorPath = FPaths::ProjectSavedDir() / TEXT("tumor.obj");

    const FString AxialPath = FPaths::ProjectSavedDir() / TEXT("axial_bottom.obj");
    const FString CoronalPath = FPaths::ProjectSavedDir() / TEXT("coronal_front.obj");
    const FString SagittalPath = FPaths::ProjectSavedDir() / TEXT("sagittal_right.obj");

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("PipeRunner::SpawnMeshesFromSaved — No World found!"));
        return;
    }

    // ---------------------------------------------------------
    // Find Rig
    // ---------------------------------------------------------
    ABrainRigActor* Rig = nullptr;

    for (TActorIterator<ABrainRigActor> It(World); It; ++It)
    {
        Rig = *It;
        break;
    }

    if (!IsValid(Rig))
    {
        UE_LOG(LogTemp, Error, TEXT("PipeRunner: No BrainRigActor found."));
        return;
    }

    AActor* ParentForMeshes = Rig;

    // ---------------------------------------------------------
    // Spawn transform = Rig transform
    // ---------------------------------------------------------
    FTransform SpawnTransform = Rig->GetActorTransform();

    // ---------------------------------------------------------
    // Helper: Spawn or reuse mesh actors
    // ---------------------------------------------------------
    auto SpawnOrReuse = [&](AProceduralObjActor*& Target, const TCHAR* Label)
        {
            if (!IsValid(Target))
            {
                Target = World->SpawnActor<AProceduralObjActor>(
                    AProceduralObjActor::StaticClass(),
                    SpawnTransform
                );

                if (!IsValid(Target))
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to spawn: %s"), Label);
                    return false;
                }

                Target->SetActorLabel(Label);
                Target->AttachToActor(Rig, FAttachmentTransformRules::KeepWorldTransform);
            }
            else
            {
                Target->AttachToActor(Rig, FAttachmentTransformRules::KeepWorldTransform);
            }

            return true;
        };

    // =============================================================
    // BRAIN
    // =============================================================
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(BrainPath, Mesh))
        {
            if (SpawnOrReuse(BrainActor, TEXT("BrainMesh")))
            {
                BrainActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
                BrainActor->ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                BrainActor->ProcMesh->bUseAsyncCooking = true;

                if (BrainMaterial)
                    BrainActor->ProcMesh->SetMaterial(0, BrainMaterial);

                Rig->BrainActor = BrainActor;
            }
        }
    }

    // =============================================================
    // TUMOR
    // =============================================================
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(TumorPath, Mesh))
        {
            if (SpawnOrReuse(TumorActor, TEXT("TumorMesh")))
            {
                TumorActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
                TumorActor->ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                TumorActor->ProcMesh->bUseAsyncCooking = true;

                if (TumorMaterial)
                    TumorActor->ProcMesh->SetMaterial(0, TumorMaterial);

                Rig->TumorActor = TumorActor;
            }
        }
    }

    // =============================================================
    // AXIAL SLICE
    // =============================================================
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(AxialPath, Mesh))
        {
            if (SpawnOrReuse(AxialActor, TEXT("AxialSlice")))
            {
                AxialActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
                AxialActor->SetActorHiddenInGame(true);

                if (BrainMaterial)
                    AxialActor->ProcMesh->SetMaterial(0, BrainMaterial);

                Rig->AxialSliceActor = AxialActor;
            }
        }
    }

    // =============================================================
    // CORONAL SLICE
    // =============================================================
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(CoronalPath, Mesh))
        {
            if (SpawnOrReuse(CoronalActor, TEXT("CoronalSlice")))
            {
                CoronalActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
                CoronalActor->SetActorHiddenInGame(true);

                if (BrainMaterial)
                    CoronalActor->ProcMesh->SetMaterial(0, BrainMaterial);

                Rig->CoronalSliceActor = CoronalActor;
            }
        }
    }

    // =============================================================
    // SAGITTAL SLICE
    // =============================================================
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(SagittalPath, Mesh))
        {
            if (SpawnOrReuse(SagittalActor, TEXT("SagittalSlice")))
            {
                SagittalActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
                SagittalActor->SetActorHiddenInGame(true);

                if (BrainMaterial)
                    SagittalActor->ProcMesh->SetMaterial(0, BrainMaterial);

                Rig->SagittalSliceActor = SagittalActor;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: All meshes spawned + assigned to BrainRigActor."));
}

void APipeRunner::CheckBackendHealth()
{
    if (IsBackendReady())
    {
        UE_LOG(LogTemp, Warning, TEXT("Backend is ready! Running pipeline..."));
        RunPipeline();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Backend not ready yet... retrying"));

    // Try again after 1 second
    FTimerHandle RetryHandle;
    GetWorld()->GetTimerManager().SetTimer(
        RetryHandle,
        this,
        &APipeRunner::CheckBackendHealth,
        1.0f,
        false
    );
}

bool APipeRunner::IsBackendReady()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL("http://127.0.0.1:8000/health");
    Request->SetVerb("GET");

    bool bReady = false;

    Request->OnProcessRequestComplete().BindLambda(
        [&bReady](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200)
            {
                bReady = true;
            }
        }
    );

    Request->ProcessRequest();

    // Give async loop time to complete
    FPlatformProcess::Sleep(0.15f);

    return bReady;
}

void APipeRunner::DeleteSavedMeshes()
{
    FString SavedDir = FPaths::ProjectSavedDir();
    IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

    UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Deleting OBJ mesh files in %s"), *SavedDir);

    // Find all OBJ files inside Saved/
    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *SavedDir, TEXT("obj"));

    // Delete each OBJ file
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

        UE_LOG(LogTemp, Warning, TEXT("PipeRunner: Mesh actors cleared."));
    }
}


