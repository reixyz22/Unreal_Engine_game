#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaEnemy.generated.h"

class AArenaProjectile;
class UHealthComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class USphereComponent;

UCLASS()
class PROJECT_RIOT_API AArenaEnemy : public AActor
{
	GENERATED_BODY()

public:
	AArenaEnemy();

	virtual void Tick(float DeltaTime) override;

	void Init(UPaperFlipbook* BodyAnim, UPaperSprite* InBeadSprite, const FLinearColor& InTint);
	bool ReceiveDamage(float Amount);
	float GetRadius() const;

protected:
	virtual void BeginPlay() override;

private:
	void Fire();
	void Die();
	FVector ComputeSeparation() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY()
	TObjectPtr<UPaperSprite> BeadSprite;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float MaxHealth = 30.f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float MoveSpeed = 140.f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float PreferredRange = 420.f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float SpreadDistance = 110.f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float SpreadStrength = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float FireInterval = 2.4f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float BeadSpeed = 320.f;

	UPROPERTY(EditAnywhere, Category = "Pyro")
	float BeadDamage = 10.f;

	FLinearColor Tint = FLinearColor::White;
	FTimerHandle FireTimer;
};
