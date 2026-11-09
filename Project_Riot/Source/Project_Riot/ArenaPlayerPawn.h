#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaPlayerPawn.generated.h"

class AArenaEnemy;
class UCameraComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
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

	bool ReceiveDamage(float Amount);
	float GetRadius() const;
	FVector GetHandLocation() const;
	void OnChainReturned();

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void StopMove();
	void TryAutoAttack();
	AArenaEnemy* FindEnemyUnderCursor() const;
	void UpdateStatusText() const;
	void UpdateAnimation(const FVector2D& Direction);
	void PlayStrike();
	void Die();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> HeldFlail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Arena")
	float ArenaHalfSize = 1024.f;

	UPROPERTY(EditAnywhere, Category = "Arena")
	float WallThickness = 64.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraViewWidth = 1280.f;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> IdleAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> RunAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> StrikeAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	FLinearColor BodyTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	TObjectPtr<UPaperSprite> FlailHandleSprite;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	TObjectPtr<UPaperSprite> ChainBallSprite;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float AutoDamage = 16.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float AutoRange = 650.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float ChainSpeed = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float ClickRadius = 90.f;

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bChainOut = false;
	double StrikeEndsAt = 0.0;
};
