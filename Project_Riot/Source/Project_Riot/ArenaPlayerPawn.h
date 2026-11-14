#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaPlayerPawn.generated.h"

class AArenaEnemy;
class UCameraComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
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

	bool ReceiveStunHit(float Amount, float StunSeconds);
	void Heal(float Amount);
	void AddMana(int32 Amount) { Mana = FMath::Min(Mana + Amount, MaxMana); }

	const UHealthComponent& GetHealth() const { return *Health; }
	int32 GetMana() const { return Mana; }
	int32 GetMaxMana() const { return MaxMana; }
	bool IsManaFull() const { return Mana >= MaxMana; }
	bool IsStunned() const;
	UPaperSprite* GetSpecialIcon() const { return GuitarSprite; }
	int32 GetSpecialCost() const { return SpecialManaCost; }

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void StopMove();
	void UpdateHover();
	void HandleRightClick();
	void UpdateMovement(float DeltaTime);
	FVector2D ComputeMoveDirection();
	void ThrowChainAt(AArenaEnemy* Target);
	bool GetCursorWorldLocation(FVector& OutLocation) const;
	AArenaEnemy* FindEnemyNear(const FVector& Point) const;
	void UpdateAnimation(const FVector2D& Direction);
	void UpdateBlink();
	void UpdateGuitar();
	void CastSpecial();
	void PlayStrike();
	void Die();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> HeldFlail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Guitar;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraViewWidth = 1280.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<UMaterialInterface> FogMaterial;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> IdleAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> RunAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> StrikeAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	FLinearColor BodyTint = FLinearColor::White;

	// Hardware cursor images, relative to Content/ and without the extension.
	UPROPERTY(EditDefaultsOnly, Category = "Look")
	FName PointerCursor = TEXT("Slate/Cursors/Nikki_Pointer");

	UPROPERTY(EditDefaultsOnly, Category = "Look")
	FName AttackCursor = TEXT("Slate/Cursors/Nikki_Sword");

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

	UPROPERTY(EditAnywhere, Category = "Special")
	TObjectPtr<UPaperSprite> GuitarSprite;

	UPROPERTY(EditAnywhere, Category = "Special")
	TObjectPtr<UPaperSprite> ShockBurstSprite;

	UPROPERTY(EditAnywhere, Category = "Special")
	int32 MaxMana = 1;

	UPROPERTY(EditAnywhere, Category = "Special")
	int32 SpecialManaCost = 1;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialDamage = 16.f;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialRadius = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialSeconds = 0.5f;

	int32 Mana = 0;
	double StunnedUntil = 0.0;
	double StrumEndsAt = 0.0;
	FVector2D MoveInput = FVector2D::ZeroVector;
	TOptional<FVector> MoveTarget;
	TWeakObjectPtr<AArenaEnemy> HoveredEnemy;
	TWeakObjectPtr<AArenaEnemy> BufferedAuto;
	bool bChainOut = false;
	double StrikeEndsAt = 0.0;
};
