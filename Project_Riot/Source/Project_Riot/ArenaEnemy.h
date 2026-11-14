#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaEnemy.generated.h"

class UHealthComponent;
class UMaterialInstanceDynamic;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class USphereComponent;

// Everything every enemy type shares: body, health, hit flash, hover outline.
// Each type (APyroEnemy, ...) adds its own movement and attacks in Tick.
UCLASS(Abstract)
class PROJECT_RIOT_API AArenaEnemy : public AActor
{
	GENERATED_BODY()

public:
	AArenaEnemy();

	virtual void Tick(float DeltaTime) override;

	bool ReceiveDamage(float Amount);
	float GetRadius() const;
	void SetHovered(bool bHovered);
	UPaperSprite* GetIcon() const;

protected:
	virtual void BeginPlay() override;
	virtual void Die();

	APawn* GetPlayer() const;
	void FaceToward(double DirectionX);
	// Every enemy moves through this, so none can leave the arena.
	void MoveWithinArena(const FVector& Offset);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	// This run's theme skin for this enemy type, if any (set in BeginPlay).
	const struct FEnemySkin* Skin = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TObjectPtr<UPaperFlipbook> BodyAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	float MaxHealth = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	float MoveSpeed = 140.f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
};
