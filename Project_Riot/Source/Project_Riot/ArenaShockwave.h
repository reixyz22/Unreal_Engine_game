#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaShockwave.generated.h"

class AArenaEnemy;
class UPaperSprite;
class UPaperSpriteComponent;

// Nikki's Q: a burst ring that expands to fill the screen, hitting each enemy once
// and erasing enemy projectiles it passes over.
UCLASS()
class PROJECT_RIOT_API AArenaShockwave : public AActor
{
	GENERATED_BODY()

public:
	AArenaShockwave();

	void Launch(UPaperSprite* BurstSprite, float InDamage, float InMaxRadius, float InSeconds);
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	TSet<TWeakObjectPtr<AArenaEnemy>> HitAlready;
	float Damage = 0.f;
	float MaxRadius = 0.f;
	float Seconds = 0.5f;
	double StartedAt = 0.0;
};
