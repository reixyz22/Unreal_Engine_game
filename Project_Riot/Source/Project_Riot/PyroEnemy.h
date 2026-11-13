#pragma once

#include "CoreMinimal.h"
#include "ArenaEnemy.h"
#include "PyroEnemy.generated.h"

class UPaperSprite;

// Ranged fodder: holds a casting range, spreads out from other enemies, floats slow beads.
UCLASS()
class PROJECT_RIOT_API APyroEnemy : public AArenaEnemy
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Fire();
	FVector ComputeSeparation() const;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	TObjectPtr<UPaperSprite> BeadSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float PreferredRange = 420.f;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float SpreadDistance = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float SpreadStrength = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float FireInterval = 2.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float BeadSpeed = 320.f;

	UPROPERTY(EditDefaultsOnly, Category = "Pyro")
	float BeadDamage = 10.f;

	FTimerHandle FireTimer;
};
