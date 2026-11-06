#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaPlayerPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UPaperSpriteComponent;
class USphereComponent;
struct FInputActionValue;

UCLASS()
class PROJECT_RIOT_API AArenaPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AArenaPlayerPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void StopMove();
	void UpdateCamera(float DeltaTime);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Arena")
	float ArenaHalfSize = 1024.f;

	UPROPERTY(EditAnywhere, Category = "Arena")
	float WallThickness = 32.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraViewWidth = 1280.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraFollowSpeed = 6.f;

	FVector2D MoveInput = FVector2D::ZeroVector;
};
