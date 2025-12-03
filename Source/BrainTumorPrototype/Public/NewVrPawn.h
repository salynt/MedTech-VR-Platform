// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "NewVrPawn.generated.h"

class ABrainRigActor;

class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class UEnhancedInputComponent;

UCLASS(Blueprintable, BlueprintType)
class BRAINTUMORPROTOTYPE_API ANewVrPawn : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANewVrPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "VR Input")
	void MovePlayer(FVector2D MovementVector);

	UFUNCTION(BlueprintCallable, Category = "VR Input")
	void TurnPlayer(float TurnValue);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	class UCameraComponent* CameraRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Brain")
	ABrainRigActor* BrainRig;

};
