#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaProjectile.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

UCLASS()
class PROJECT_RIOT_API AArenaProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArenaProjectile();

	virtual void Tick(float DeltaTime) override;

	void Launch(UPaperSprite* InSprite, const FLinearColor& Tint, const FVector& InVelocity, float InDamage);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.f;
	float HitRadius = 10.f;
};
