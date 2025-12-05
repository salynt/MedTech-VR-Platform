#include "NewVrPawn.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ANewVrPawn::ANewVrPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // Allow yaw rotation from controller stick
    bUseControllerRotationYaw = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
}

void ANewVrPawn::BeginPlay()
{
    Super::BeginPlay();

    // -----------------------------------------
    // AUTO-DETECT CAMERA COMPONENT
    // -----------------------------------------
    if (!CameraRef)
    {
        CameraRef = FindComponentByClass<UCameraComponent>();

        if (CameraRef)
        {
            UE_LOG(LogTemp, Log,
                TEXT("NewVrPawn: Auto-detected Camera Component: %s"),
                *CameraRef->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("NewVrPawn: No CameraComponent found! Movement will not use head direction!"));
        }
    }

    // -----------------------------------------
    // SETUP ENHANCED INPUT (Mapping Context)
    // -----------------------------------------
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Log, TEXT("Mapping context added."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is NOT assigned!"));
            }
        }
    }
}

void ANewVrPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ANewVrPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // MOVE
        if (MoveAction)
        {
            EI->BindAction(
                MoveAction,
                ETriggerEvent::Triggered,
                this,
                &ANewVrPawn::MovePlayer
            );
        }

        // TURN
        if (TurnAction)
        {
            EI->BindAction(
                TurnAction,
                ETriggerEvent::Triggered,
                this,
                &ANewVrPawn::TurnPlayer
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent not found!"));
    }
}

void ANewVrPawn::MovePlayer(const FInputActionValue& Value)
{
    FVector2D Input = Value.Get<FVector2D>();
    if (Input.IsNearlyZero()) return;

    if (!CameraRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraRef still null in MovePlayer"));
        return;
    }

    const float Yaw = CameraRef->GetComponentRotation().Yaw;
    const FRotator HeadYaw(0.f, Yaw, 0.f);

    const FVector Forward = FRotationMatrix(HeadYaw).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(HeadYaw).GetUnitAxis(EAxis::Y);

    AddMovementInput(Forward, Input.Y);
    AddMovementInput(Right, Input.X);
}

void ANewVrPawn::TurnPlayer(const FInputActionValue& Value)
{
    float Input = Value.Get<float>();
    const float TurnSensitivity = 5.0f;

    if (FMath::Abs(Input) > KINDA_SMALL_NUMBER)
    {
        AddControllerYawInput(Input * TurnSensitivity);
    }
}
