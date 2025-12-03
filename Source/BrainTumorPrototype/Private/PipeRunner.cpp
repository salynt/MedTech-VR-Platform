#include "PipeRunner.h"
#include "ProceduralObjActor.h"
#include "ObjMeshLoader.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

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
    // Get full path to StartBackend.bat
    FString BackendScript = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectDir() / TEXT("StartBackend.bat")
    );

    UE_LOG(LogTemp, Warning, TEXT("Launching backend script at: %s"), *BackendScript);

    // Launch backend one time
    FPlatformProcess::CreateProc(
        *BackendScript,
        nullptr,
        true,    // launch detached
        false,
        false,
        nullptr,
        0,
        TEXT("C:/Programming/mri-ai-backend"),
        nullptr
    );

    UE_LOG(LogTemp, Warning, TEXT("Backend launched. Waiting for health check..."));
   
    
    // Begin backend readiness loop
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
    //const FString SaveHalf_AxialTop = FPaths::ProjectSavedDir() / TEXT("axial_top.obj");
    const FString SaveHalf_AxialBottom = FPaths::ProjectSavedDir() / TEXT("axial_bottom.obj");
    const FString SaveHalf_CoronalFront = FPaths::ProjectSavedDir() / TEXT("coronal_front.obj");
    //const FString SaveHalf_CoronalBack = FPaths::ProjectSavedDir() / TEXT("coronal_back.obj");
    //const FString SaveHalf_SagittalLeft = FPaths::ProjectSavedDir() / TEXT("sagittal_left.obj");
    const FString SaveHalf_SagittalRight = FPaths::ProjectSavedDir() / TEXT("sagittal_right.obj");

    DownloadMesh(BrainUrl, SaveBrain);
    DownloadMesh(TumorUrl, SaveTumor);
    DownloadMesh(AxialBottomUrl, SaveHalf_AxialBottom);
    //DownloadMesh(AxialTopUrl, SaveHalf_AxialTop);
    DownloadMesh(CoronalFrontUrl, SaveHalf_CoronalFront);
    //DownloadMesh(CoronalBackUrl, SaveHalf_CoronalBack);
    //DownloadMesh(SagittalLeftUrl, SaveHalf_SagittalLeft);
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
    const FString AxialBottomPath = FPaths::ProjectSavedDir() / TEXT("axial_bottom.obj");
    const FString CoronalFrontPath = FPaths::ProjectSavedDir() / TEXT("coronal_front.obj");
    const FString SagRightPath = FPaths::ProjectSavedDir() / TEXT("sagittal_right.obj");

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("PipeRunner::SpawnMeshesFromSaved: No World found!"));
        return;
    }

    // Base transform for spawned meshes
    FTransform SpawnTransform;
    SpawnTransform.SetLocation(BrainDestination);
    SpawnTransform.SetRotation(BrainRotation.Quaternion());
    SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

    // Small helper to spawn or reuse an AProceduralObjActor
    auto SpawnOrReuseActor = [&](AProceduralObjActor*& OutActor, const TCHAR* Label) -> bool
        {
            if (!IsValid(OutActor))
            {
                OutActor = World->SpawnActor<AProceduralObjActor>(
                    AProceduralObjActor::StaticClass(),
                    SpawnTransform
                );

                if (!IsValid(OutActor))
                {
                    UE_LOG(LogTemp, Error, TEXT("PipeRunner::SpawnOrReuseActor: FAILED to spawn %s"), Label);
                    return false;
                }

                OutActor->SetActorLabel(Label);

                // Parent to this PipeRunner so moving PipeRunner moves the meshes
                OutActor->AttachToActor(
                    this,
                    FAttachmentTransformRules::KeepWorldTransform
                );
            }

            return true;
        };

    // ------------------------
    // BRAIN MESH
    // ------------------------
    {
        FLoadedObjMesh Mesh;
        if (!FObjMeshLoader::LoadFromFile(BrainPath, Mesh))
        {
            UE_LOG(LogTemp, Error, TEXT("FAILED loading brain.obj at %s"), *BrainPath);
        }
        else
        {
            if (!SpawnOrReuseActor(BrainActor, TEXT("BrainMesh")))
            {
                return; // can't continue safely
            }

            if (!IsValid(BrainActor->ProcMesh))
            {
                UE_LOG(LogTemp, Error, TEXT("BrainActor->ProcMesh is NULL! Check AProceduralObjActor constructor."));
                return;
            }

            // Actually build the geometry from the loaded mesh data
            BrainActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);

            BrainActor->ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            BrainActor->ProcMesh->bUseAsyncCooking = true;

            if (BrainMaterial)
            {
                BrainActor->ProcMesh->SetMaterial(0, BrainMaterial);
            }

            UE_LOG(LogTemp, Log, TEXT("Spawned/updated BrainMesh from %s"), *BrainPath);
        }
    }

    // ------------------------
    // TUMOR MESH
    // ------------------------
    {
        FLoadedObjMesh Mesh;
        if (!FObjMeshLoader::LoadFromFile(TumorPath, Mesh))
        {
            UE_LOG(LogTemp, Error, TEXT("FAILED loading tumor.obj at %s"), *TumorPath);
        }
        else
        {
            if (!SpawnOrReuseActor(TumorActor, TEXT("TumorMesh")))
            {
                return;
            }

            if (!IsValid(TumorActor->ProcMesh))
            {
                UE_LOG(LogTemp, Error, TEXT("TumorActor->ProcMesh is NULL! Check AProceduralObjActor constructor."));
                return;
            }

            TumorActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);

            TumorActor->ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            TumorActor->ProcMesh->bUseAsyncCooking = true;

            if (TumorMaterial)
            {
                TumorActor->ProcMesh->SetMaterial(0, TumorMaterial);
            }

            UE_LOG(LogTemp, Log, TEXT("Spawned/updated TumorMesh from %s"), *TumorPath);
        }
    }

    // ------------------------
    // (OPTIONAL) AXIAL BOTTOM / CORONAL / SAG RIGHT
    // 
    // ------------------------
    /*
    {
        FLoadedObjMesh Mesh;
        if (FObjMeshLoader::LoadFromFile(AxialBottomPath, Mesh))
        {
            if (!SpawnOrReuseActor(AxialActor, TEXT("AxialBottomMesh")))
            {
                return;
            }

            if (!IsValid(AxialActor->ProcMesh))
            {
                UE_LOG(LogTemp, Error, TEXT("AxialActor->ProcMesh is NULL!"));
                return;
            }

            AxialActor->BuildFromLoadedMesh(Mesh.Vertices, Mesh.Triangles);
            AxialActor->ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FAILED loading axial_bottom.obj at %s"), *AxialBottomPath);
        }
    }
    */
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

//
// CHECK IF BACKEND IS RUNNING
//
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

