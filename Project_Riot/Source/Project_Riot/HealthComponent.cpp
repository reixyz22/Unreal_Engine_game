#include "HealthComponent.h"

#include "Engine/World.h"

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

bool UHealthComponent::IsInvulnerable() const
{
	return GetWorld()->GetTimeSeconds() < InvulnerableUntil;
}

bool UHealthComponent::ApplyDamage(float Amount)
{
	if (IsDead() || IsInvulnerable())
	{
		return false;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	InvulnerableUntil = GetWorld()->GetTimeSeconds() + InvulnerableSeconds;

	if (IsDead())
	{
		OnDepleted.Broadcast();
	}
	return true;
}
