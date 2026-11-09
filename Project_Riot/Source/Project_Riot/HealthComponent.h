#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnHealthDepleted);

UCLASS(ClassGroup = (Arena), meta = (BlueprintSpawnableComponent))
class PROJECT_RIOT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool ApplyDamage(float Amount);
	void SetMaxHealth(float InMaxHealth) { MaxHealth = InMaxHealth; }

	bool IsInvulnerable() const;
	bool IsDead() const { return CurrentHealth <= 0.f; }
	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }

	FOnHealthDepleted OnDepleted;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Health")
	float InvulnerableSeconds = 0.5f;

	float CurrentHealth = 0.f;
	double InvulnerableUntil = 0.0;
};
