// Fill out your copyright notice in the Description page of Project Settings.


#include "NewVrPawn.h"
#include "BrainRigActor.h"
#include "MotionControllerComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"


ANewVrPawn::ANewVrPawn()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ANewVrPawn::BeginPlay()
{
	Super::BeginPlay();


}

// Called every frame
void ANewVrPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ANewVrPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// Function to handle movement input
void ANewVrPawn::MovePlayer(FVector2D MovementVector)
{
	if (!Controller || MovementVector.IsNearlyZero()) return;

	if (!CameraRef)
	{
		UE_LOG(LogTemp, Error, TEXT("CameraRef is null in MovePlayer!"));
		return;
	}
	
	const FRotator YawRotation(0.f, CameraRef->GetComponentRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
	UE_LOG(LogTemp, Warning, TEXT("Thumbstick X: %f, Y: %f"), MovementVector.X, MovementVector.Y);
}

// Function to handle turning input
void ANewVrPawn::TurnPlayer(float TurnValue)
{
	const float TurnSensitivity = 5.0f;

	if (FMath::Abs(TurnValue) > KINDA_SMALL_NUMBER)
	{
		AddControllerYawInput(TurnValue * TurnSensitivity);
	}
}
