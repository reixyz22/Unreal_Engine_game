#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaArrow.generated.h"

class AArenaEnemy;
class UPaperSprite;
class UPaperSpriteComponent;

// Carl's arrow. It flies straight until it hits one enemy or the arena wall.
// With bounces left it ricochets instead: off walls like a mirror, off enemies
// away from where it struck. It can't re-hit the enemy it just bounced off
// until it has bounced off something else.
UCLASS()
class PROJECT_RIOT_API AArenaArrow : public AActor
{
	GENERATED_BODY()

public:
	AArenaArrow();

	void Fire(UPaperSprite* ArrowSprite, const FVector& Direction, float InSpeed, float InDamage, int32 InBounces);
	virtual void Tick(float DeltaTime) override;

private:
	void Aim();
	AArenaEnemy* FindEnemyHit() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.f;
	int32 BouncesLeft = 0;
	TWeakObjectPtr<AArenaEnemy> JustBouncedOff;
};
