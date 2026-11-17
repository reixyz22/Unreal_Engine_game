#pragma once

#include "CoreMinimal.h"
#include "ArenaStats.generated.h"

UENUM()
enum class EDamageType : uint8
{
	Physical,  // scales with Attack Damage, reduced by Armor
	Magic,     // scales with Ability Power, reduced by Magic Resist
};

// The stat block every hero and enemy has, modelled on League of Legends.
USTRUCT(BlueprintType)
struct FArenaStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Armor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MagicResist = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 400.f;

	// An all-zero block, for bonuses.
	static FArenaStats Zero()
	{
		FArenaStats Stats;
		Stats.MaxHealth = Stats.AttackDamage = Stats.MoveSpeed = 0.f;
		return Stats;
	}

	// Adds Count copies of Bonus (shop upgrades stack this way).
	void Add(const FArenaStats& Bonus, int32 Count)
	{
		MaxHealth += Bonus.MaxHealth * Count;
		AttackDamage += Bonus.AttackDamage * Count;
		AbilityPower += Bonus.AbilityPower * Count;
		Armor += Bonus.Armor * Count;
		MagicResist += Bonus.MagicResist * Count;
		MoveSpeed += Bonus.MoveSpeed * Count;
	}

	// League's formula: each point of resistance adds 1% effective health,
	// so 100 armor halves physical damage.
	float Mitigate(float RawDamage, EDamageType Type) const
	{
		const float Resist = FMath::Max(0.f, Type == EDamageType::Physical ? Armor : MagicResist);
		return RawDamage * 100.f / (100.f + Resist);
	}
};
