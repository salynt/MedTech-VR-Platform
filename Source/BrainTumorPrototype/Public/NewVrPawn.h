#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "NewVrPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;

UCLASS()
class BRAINTUMORPROTOTYPE_API ANewVrPawn : public ACharacter
{
    GENERATED_BODY()

public:
    ANewVrPawn();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;

public:

    /** Auto-detected camera on pawn */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
    UCameraComponent* CameraRef;

    /** Input mapping context */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    /** Move action (left stick) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    /** Turn action (right stick X axis) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* TurnAction;

    /** Movement handler */
    UFUNCTION()
    void MovePlayer(const FInputActionValue& Value);

    /** Turning handler */
    UFUNCTION()
    void TurnPlayer(const FInputActionValue& Value);
};
