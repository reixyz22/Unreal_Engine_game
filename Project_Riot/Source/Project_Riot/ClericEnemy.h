#pragma once

#include "CoreMinimal.h"
#include "ArenaEnemy.h"
#include "ClericEnemy.generated.h"

class UPaperSprite;
class USoundBase;

// Slow caster: keeps its distance and fires an arena-wide stun beam at the player.
// Always drops a shard: mana, or health if the player's mana is already full.
UCLASS()
class PROJECT_RIOT_API AClericEnemy : public AArenaEnemy
{
	GENERATED_BODY()

public:
	AClericEnemy();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void Die() override;

private:
	void CastBeam();

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	TObjectPtr<UPaperSprite> BeamSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	TObjectPtr<UPaperSprite> ManaShardSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	TObjectPtr<UPaperSprite> HealthShardSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	TObjectPtr<USoundBase> BeamChargeSound;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	TObjectPtr<USoundBase> BeamFireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float PreferredRange = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float CastInterval = 4.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float WarningSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float BeamSeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float BeamWidth = 44.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float BeamDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cleric")
	float StunSeconds = 0.9f;

	FTimerHandle CastTimer;
	double CastingUntil = 0.0;
};
